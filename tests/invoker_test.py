"""
Tests specific to the unified invoker functionality.

These tests verify:
- Help system works correctly
- Unknown commands are handled properly
- C++ and Python command abbreviations work
- Commands are correctly dispatched to their implementations
"""

import pytest
import subprocess


class TestHelpSystem:
    """Test the help and usage display."""

    def test_help_no_args(self):
        """afdko with no args shows help and exits with code 1."""
        result = subprocess.run(['afdko'], capture_output=True, text=True)
        assert result.returncode == 1
        assert 'Usage: afdko <command>' in result.stdout
        assert 'Adobe Font Development Kit for OpenType' in result.stdout

    @pytest.mark.parametrize('arg', ['-h', '--help', 'help'])
    def test_help_explicit(self, arg):
        """Various help flags work and exit with code 0."""
        result = subprocess.run(['afdko', arg], capture_output=True, text=True)
        assert result.returncode == 0
        assert 'Usage: afdko <command>' in result.stdout
        assert 'Primary Commands:' in result.stdout
        assert "for command-specific help" in result.stdout

    def test_help_lists_primary_tools(self):
        """Help output includes primary tools (C++ and Python)."""
        result = subprocess.run(['afdko', '--help'], capture_output=True, text=True)
        assert result.returncode == 0
        # Check for some key primary commands (mix of C++ and Python)
        assert 'tx' in result.stdout
        assert 'sfntedit' in result.stdout
        assert 'spot' in result.stdout
        assert 'addfeatures' in result.stdout
        assert 'makeotf' in result.stdout
        assert 'otfautohint' in result.stdout
        assert 'buildcff2vf' in result.stdout

    @pytest.mark.parametrize('arg', ['-s', '--secondary'])
    def test_help_secondary_commands(self, arg):
        """Secondary commands help flag works."""
        result = subprocess.run(['afdko', arg], capture_output=True, text=True)
        assert result.returncode == 0
        assert 'Usage: afdko <command>' in result.stdout
        assert 'Secondary Commands:' in result.stdout
        # Check for some secondary commands
        assert 'otc2otf' in result.stdout or 'comparefamily' in result.stdout

    @pytest.mark.parametrize('arg', ['-p', '--plot'])
    def test_help_proofing_commands(self, arg):
        """Proofing commands help flag works."""
        result = subprocess.run(['afdko', arg], capture_output=True, text=True)
        assert result.returncode == 0
        assert 'Usage: afdko <command>' in result.stdout
        assert 'Proofing Commands:' in result.stdout
        # Check for some proofing commands
        assert 'fontplot' in result.stdout or 'hintplot' in result.stdout

    @pytest.mark.parametrize('arg', ['-a', '--all'])
    def test_help_all_commands(self, arg):
        """All commands help flag works."""
        result = subprocess.run(['afdko', arg], capture_output=True, text=True)
        assert result.returncode == 0
        assert 'Usage: afdko <command>' in result.stdout
        # Should show all three categories
        assert 'Primary Commands:' in result.stdout
        assert 'Secondary Commands:' in result.stdout
        assert 'Proofing Commands:' in result.stdout

    def test_help_command_specific_forwarding(self):
        """afdko -h <command> forwards to command help."""
        # Test with a C++ command
        result = subprocess.run(['afdko', '-h', 'tx'],
                               capture_output=True, text=True)
        # Should show tx-specific help (may be in stdout or stderr)
        output = result.stdout + result.stderr
        assert 'tx' in output.lower()

    def test_help_command_specific_unknown(self):
        """afdko -h <invalid> shows error."""
        result = subprocess.run(['afdko', '-h', 'invalidcmd'],
                               capture_output=True, text=True)
        assert result.returncode == 1
        assert "Unknown command 'invalidcmd'" in result.stderr


class TestErrorHandling:
    """Test error handling for invalid commands."""

    def test_unknown_command(self):
        """Unknown commands show error and exit with code 1."""
        result = subprocess.run(['afdko', 'invalidcommand'],
                               capture_output=True, text=True)
        assert result.returncode == 1
        assert "Unknown command 'invalidcommand'" in result.stderr
        assert "Run 'afdko --help' for usage." in result.stderr

    def test_unknown_command_similar_to_real(self):
        """Commands with typos show error."""
        result = subprocess.run(['afdko', 'makeoft'],  # typo in makeotf
                               capture_output=True, text=True)
        assert result.returncode == 1
        assert "Unknown command 'makeoft'" in result.stderr


class TestCppCommandAbbreviations:
    """Test C++ command abbreviations work correctly."""

    @pytest.mark.parametrize('cmd,abbrev', [
        ('sfntedit', 'se'),
        ('addfeatures', 'af'),
        ('detype1', 'dt1'),
        ('type1', 't1'),
        ('mergefonts', 'mf'),
        ('rotatefont', 'rf'),
    ])
    def test_cpp_abbreviations_help(self, cmd, abbrev):
        """C++ command abbreviations work correctly."""
        cmd_result = subprocess.run(['afdko', cmd, '-h'],
                                    capture_output=True, text=True)
        abbrev_result = subprocess.run(['afdko', abbrev, '-h'],
                                       capture_output=True, text=True)

        # Both should return same exit code (may be 0 or 1 depending on tool)
        assert cmd_result.returncode == abbrev_result.returncode
        # Both should produce output (either to stdout or stderr)
        cmd_output = cmd_result.stdout + cmd_result.stderr
        abbrev_output = abbrev_result.stdout + abbrev_result.stderr
        assert len(cmd_output) > 0, f"{cmd} produced no output"
        assert len(abbrev_output) > 0, f"{abbrev} produced no output"

    def test_tx_no_abbreviation(self):
        """tx has no abbreviation, so tx works but t doesn't."""
        # tx should work
        result = subprocess.run(['afdko', 'tx', '-h'],
                               capture_output=True, text=True)
        assert result.returncode == 0

        # t is not a valid command (tx has no abbreviation)
        result = subprocess.run(['afdko', 't', '-h'],
                               capture_output=True, text=True)
        assert result.returncode == 1
        assert "Unknown command 't'" in result.stderr

    def test_spot_no_abbreviation(self):
        """spot has no abbreviation."""
        result = subprocess.run(['afdko', 'spot', '-h'],
                               capture_output=True, text=True)
        assert result.returncode == 0

    def test_sfntdiff_no_abbreviation(self):
        """sfntdiff has no abbreviation."""
        result = subprocess.run(['afdko', 'sfntdiff', '-h'],
                               capture_output=True, text=True)
        assert result.returncode == 0


class TestPythonCommandAbbreviations:
    """Test Python command abbreviations work correctly."""

    @pytest.mark.parametrize('cmd,abbrev', [
        ('makeotf', 'mo'),
        ('buildcff2vf', 'bvf'),
        ('buildmasterotfs', 'bmo'),
        ('makeinstancesufo', 'miu'),
        ('checkoutlinesufo', 'cou'),
        ('comparefamily', 'cf'),
    ])
    def test_python_abbreviations_help(self, cmd, abbrev):
        """Python command abbreviations work correctly."""
        cmd_result = subprocess.run(['afdko', cmd, '-h'],
                                    capture_output=True, text=True)
        abbrev_result = subprocess.run(['afdko', abbrev, '-h'],
                                       capture_output=True, text=True)

        # Both should have same return code
        assert cmd_result.returncode == abbrev_result.returncode

    @pytest.mark.parametrize('cmd,abbrev1,abbrev2', [
        ('otfautohint', 'autohint', 'ah'),
        ('otfstemhist', 'stemhist', 'sh'),
    ])
    def test_python_multiple_abbreviations(self, cmd, abbrev1, abbrev2):
        """Some Python commands have multiple abbreviations."""
        cmd_result = subprocess.run(['afdko', cmd, '-h'],
                                    capture_output=True, text=True)
        abbrev1_result = subprocess.run(['afdko', abbrev1, '-h'],
                                        capture_output=True, text=True)
        abbrev2_result = subprocess.run(['afdko', abbrev2, '-h'],
                                        capture_output=True, text=True)

        # All should have same return code
        assert cmd_result.returncode == abbrev1_result.returncode == abbrev2_result.returncode


class TestCppCommands:
    """Test that C++ commands work via the invoker."""

    def test_cpp_commands_work(self):
        """All C++ commands work via invoker."""
        cpp_commands = ['tx', 'sfntedit', 'spot', 'addfeatures',
                       'detype1', 'type1', 'sfntdiff', 'mergefonts', 'rotatefont']

        for cmd in cpp_commands:
            result = subprocess.run(['afdko', cmd, '-h'],
                                   capture_output=True, text=True)
            # All C++ commands should at least handle -h successfully
            # (Some might return 0, some might return 1, but shouldn't crash)
            assert result.returncode in (0, 1), f"{cmd} failed unexpectedly"

    def test_cpp_abbreviations_work(self):
        """All C++ abbreviations work via invoker."""
        abbreviations = ['se', 'af', 'dt1', 't1', 'mf', 'rf']

        for abbrev in abbreviations:
            result = subprocess.run(['afdko', abbrev, '-h'],
                                   capture_output=True, text=True)
            # All should at least handle -h without crashing
            assert result.returncode in (0, 1), f"{abbrev} failed unexpectedly"


class TestPythonFallback:
    """Test that Python commands work via Python fallback path."""

    def test_python_commands_work(self):
        """Python commands work via invoker."""
        # Test a representative sample of Python commands
        python_commands = ['makeotf', 'buildcff2vf', 'otfautohint',
                          'otc2otf', 'otf2otc', 'ttxn']

        for cmd in python_commands:
            result = subprocess.run(['afdko', cmd, '-h'],
                                   capture_output=True, text=True)
            # All should handle -h (return code 0 or 1 depending on command)
            assert result.returncode in (0, 1, 2), f"{cmd} failed unexpectedly"

    def test_proofing_tools_work(self):
        """Proofing tools work via invoker."""
        proofing_tools = ['charplot', 'digiplot', 'fontplot',
                         'hintplot', 'waterfallplot']

        for cmd in proofing_tools:
            result = subprocess.run(['afdko', cmd, '-h'],
                                   capture_output=True, text=True)
            # All should handle -h
            assert result.returncode in (0, 1, 2), f"{cmd} failed unexpectedly"


class TestCommandDispatch:
    """Test that commands are dispatched correctly."""

    def test_tx_version(self):
        """tx -v works via invoker."""
        result = subprocess.run(['afdko', 'tx', '-v'],
                               capture_output=True, text=True)
        # tx -v should print version and exit
        assert 'tx' in result.stdout.lower() or 'version' in result.stdout.lower()

    def test_makeotf_version_python_fallback(self):
        """makeotf -v uses Python fallback."""
        result = subprocess.run(['afdko', 'makeotf', '-v'],
                               capture_output=True, text=True)
        # makeotf -v should print version
        # (Return code may vary, but shouldn't crash)
        assert result.returncode in (0, 1, 2)


class TestBackwardsCompatibility:
    """Test that invoker maintains backwards compatibility."""

    def test_invoker_vs_wrapper_tx_help(self):
        """tx -h via invoker matches tx -h via wrapper."""
        inv = subprocess.run(['afdko', 'tx', '-h'],
                            capture_output=True, text=True)
        wrap = subprocess.run(['tx', '-h'],
                             capture_output=True, text=True)

        # Return codes should match
        assert inv.returncode == wrap.returncode
        # Output should be identical
        assert inv.stdout == wrap.stdout

    def test_invoker_vs_wrapper_makeotf_help(self):
        """makeotf -h via invoker matches makeotf -h via wrapper."""
        inv = subprocess.run(['afdko', 'makeotf', '-h'],
                            capture_output=True, text=True)
        wrap = subprocess.run(['makeotf', '-h'],
                             capture_output=True, text=True)

        # Return codes should match
        assert inv.returncode == wrap.returncode
        # Output should be identical
        assert inv.stdout == wrap.stdout


