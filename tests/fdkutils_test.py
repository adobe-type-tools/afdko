import argparse
import pytest

from afdko.fdkutils import validate_path


def test_validate_path():
    with pytest.raises(argparse.ArgumentTypeError):
        validate_path('not_a_file_or_folder')
