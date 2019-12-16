__copyright__ = """Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

__doc__ = """ This module provides a set of functions for working with glyph name
dictionaries. In particular, to load the latest set of recommended Adobe Glyph
names and Unicode values, from one of the scripts in
python/afdko, the set of commands is:
	import agd
	import os
    import fdkutils
	resources_dir = fdkutils.get_resources_dir()
	kAGD_TXTPath = os.path.join(resources_dir, "AGD.txt")
	fp = open(kAGD_TXTPath, "rU")
	agdTextPath = fp.read()
	fp.close()
	gAGDDict = agd.dictionary(agdTextPath)
"""

import re


#===========================================================
# A glyph entry from a Glyph Dictionary

re_entry = re.compile(r'(?m)^[\t ]+(\S+?): +(\S.*?)$') # regexp: data entry line
re_name = re.compile(r'[A-Za-z0-9_\.]+') # regexp: glyph name
class glyph:
	def __init__(self, name):
		self.name = name
		self.uni = None # The Unicode
		self.fin = None # The final name
		self.ali = [] # list of name aliases
		self.sub = [] # list of substitutions
		self.set = [] # list of glyph set IDs
		self.min = None # minuscule
		self.maj = None # majuscule
		self.cmp = None # components used to create this composite glyph
		self.other = {} # Hash of any unknown tags

	# add entry data from input text:
	def parse(self, intext):
		ee = re_entry.findall(intext) # get all data entries
		if len(ee) < 1: return False # return if no data entries
		for e in ee:
			k, x = e # get key and its data
			if k == 'uni':
				self.uni = re.compile(r'[0-9A-F]{4}').search(x).group() # Unicode value
			elif k == 'fin': self.fin = re_name.search(x).group() # final name
			elif k == 'ali':
				for a in re_name.findall(x): self.ali.append(a) # name aliases
			elif k == 'sub': self.sub = x.split() # substitutions
			elif k == 'set': self.set.extend(x.split()) # glyph sets
			elif k == 'min': self.min = re_name.search(x).group() # minuscule
			elif k == 'maj': self.maj = re_name.search(x).group() # majuscule
			elif k == 'cmp': self.cmp = x # components
			else: self.other[k] = x # any unknown entry key
		self.check() # re-check glyph data
		return True

	# Check a glyph for internal conflicts, problems, etc.
	def check(self):
		m = [] # holds reports of what was fixed
		u = re.compile(r'uni([0-9A-F]{4})$') # Unicode value regexp
		n = {}
		# process aliases: remove duplicates, uninames, final names:
		for a in self.ali:
			# Remove alias if it matches the index name:
			if a == self.name:
				m.append("Removed alias '%s' from glyph '%s' (same as index name)." % (a, self.name))
				continue
			# Remove alias if it matches the final name:
			elif self.fin and a == self.fin:
				m.append("Removed alias '%s' from glyph '%s' (same as final name)." % (a, self.name))
				continue
			# Remove alias if it is a uni-name:
			elif u.match(a):
				m.append("Removed alias '%s' from glyph '%s' (uni-name for Unicode value)." % (a, self.name))
				continue
			else: n[a] = 1 # pass the alias
		self.ali = sorted(n.keys()) # sorted list of passed aliases
		# Look for Unicode values in glyph names:
		if not self.uni:
			if self.fin and u.match(self.fin):
				m.append("Found Unicode value in final name '%s' of glyph '%s'." % (self.fin, self.name))
				self.uni = u.match(self.fin).group(1) # Get Unicode from final name
			else:
				for a in self.ali:
					if u.match(a):
						m.append("Found Unicode value in alias '%s' of glyph '%s'." % (a, self.name))
						self.uni = u.match(a).group(1)
						break
		return m

	# Return a list of all glyph names/aliases, including uni-names and final names
	def aliases(self, addglyphs=[]):
		self.check()
		n = {}
		n[self.name] = 1
		if self.fin: n[self.fin] = 1
		if self.uni: n[self.uniname()] = 1
		for i in self.ali: n[i] = 1 # add the glyph's own alias names
		for i in addglyphs: n[i] = 1 # add any additional alias names
		nn = sorted(n.keys())
		return nn

	# return a uni-name for the glyph's Unicode value:
	def uniname(self):
		if self.uni: return "uni%s"%self.uni
		else: return None

#---------------------------------------
# A Glyph Dictionary

class dictionary:
	def __init__(self, intext=None):
		self.list = [] # Ordered list of glyph names
		self.glyphs = {} # Dictionary of glyph names and parents
		self.index = {} # Hash of all glyph/alias names
		self.unicode = {} # Hash of Unicode values
		self.messages = [] # A list of error/warning messages
		if intext: self.parse(intext)

	# Read text, convert to glyph entries, and add to dictionary:
	def parse(self, intext, priority=1):
		intext += '\n' # ensure the file ends with a newline for grep purposes
		re_entry = re.compile(r'(?m)^([A-Za-z0-9\._]+)\n((?:[\t ]+\S.*?\n)*)') # regexp for glyph entry text
		ee = re_entry.findall(intext) # find all glyph entries
		if len(ee) < 1: return False # return if no entries found
		for e in ee:
			gname, ginfo = e
			g = glyph(gname) # create new glyph object
			g.parse(ginfo)
			self.add(g, priority)
		return True

	# Find a glyph via index lookup, or by Unicode value:
	def glyph(self, n):
		if not n: return None
		elif n in self.index: return self.glyphs[self.index[n]] # lookup by glyph name
		elif n in self.unicode: return self.glyphs[self.unicode[n]] # lookup by Unicode value
		else: return None

	# Remove an existing glyph entry
	def remove(self, n):
		if n in self.glyphs:
			g = self.glyphs[n]
			for a in g.aliases():
				if a in self.index: del self.index[a] # remove all index entries
			if g.uni and g.uni in self.unicode: del self.unicode[g.uni] # remove unicode entry
			del self.glyphs[n] # delete the glyph entry
			i = self.list.index(n) # get order position of glyph
			del self.list[i] # remove glyph name from order list
			return i
		else: return None

	# Add a glyph object to the dictionary and perform cross-checking;
	# merge incoming data according to specified priority:
	#  priority=3: New entries completely replace existing entries
	#  priority=2: New data merged, override existing data
	#  priority=1: New data merged, defer to existing data
	#  priority=0: New entries defer entirely to existing entries
	def add(self, g, priority=1):
		m = self.messages
		addplace = None # position of added glyph in glyph order
		g.check() # check glyph before adding

		# (t) The existing glyph, if any
		t = self.glyph(g.name) or self.glyph(g.uni) # the target glyph which matches the incoming glyph (g) (or None)
		for a in g.aliases():
			if t: break
			t = self.glyph(a)

		# if a target glyph (t) is found:
		if t:
			m.append("Merged new glyph '%s' with existing glyph '%s' (priority %s)." % (g.name, t.name, priority))
			g.ali = t.aliases(g.aliases()) # append new aliases
			g.name = t.name
			if t.uni: g.uni = t.uni # Unicode value
			if g.fin and not g.fin in g.ali: g.ali.append(g.fin) # copy final name to aliases
			if t.fin: g.fin = t.fin # final name
			if t.min: g.min = t.min # minuscule
			if t.maj: g.maj = t.maj # majuscule
			if t.cmp: g.cmp = t.cmp # majuscule
			for i in t.other: g.other[i] = t.other[i]
			addplace = self.remove(t.name) # remove the conflicting glyph

		else: m.append("Adding new glyph: %s" % g.name)

		# Add the glyph to the dictionary:
		if addplace is None: self.list.append(g.name)
		else: self.list.insert(addplace, g.name) # replace the old glyph with the new glyph in the order list
		self.glyphs[g.name] = g
		for a in g.aliases(): self.index[a] = g.name # add aliases to index (including self, uni-name and final name)
		if g.uni: self.unicode[g.uni] = g.name  # add the Unicode value to the Unicode index

		return True
