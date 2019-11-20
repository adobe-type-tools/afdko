import sys

with open(sys.argv[1]) as f:
    out_lines = []
    for line in f.readlines():
        if '\t' in line:
            items = line.split('\t')
            if items[0] == 'Count':
                out_lines.append(f"count    {items[1].lower()}    glyphs")
            else:
                gl = eval(items[2])
                out_lines.append(f'{items[0]:>5}{items[1]:>9}    [{" ".join(sorted(gl))}]')  # noqa: E501
        else:
            out_lines.append(line.strip())

with open(sys.argv[1], 'w') as f:
    f.write("\n".join(out_lines))
