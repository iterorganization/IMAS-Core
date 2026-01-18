import os

import pytest

import imas_core

lowlevel = imas_core._al_lowlevel
imasdef = imas_core.imasdef


def create_temp_ids_file(content: str, suffix: str = "") -> str:
  tmpdir = os.environ.get("TEST_TMPDIR", ".")
  filename = os.path.join(tmpdir, f"test_temp{suffix}.ids")
  with open(filename, "w") as f:
    f.write(content)
  return filename


class TestAsciiBackendStatePersistence:
  """Test that reading two IDS files in sequence correctly clears state.

  This is a Python version of the C++ backend_regression_test.cpp that verifies
  the AsciiBackend properly clears its internal stringstream state between
  reads.
  """

  def test_state_persistence(self):
    content1 = "test_temp_state1/data1\n\ttype: 51 (integer)\n\tdim: 0\n123\n"
    create_temp_ids_file(content1, "_state1")

    content2 = "test_temp_state2/data2\n\ttype: 51 (integer)\n\tdim: 0\n456\n"
    create_temp_ids_file(content2, "_state2")

    tmpdir = os.environ.get("TEST_TMPDIR", ".")
    uri = f"imas:ascii?path={tmpdir}"

    status, de_ctx = lowlevel.al_begin_dataentry_action(uri, imasdef.OPEN_PULSE)
    assert status >= 0, f"Failed to open pulse: status={status}"

    status, op_ctx1 = lowlevel.al_begin_global_action(
        de_ctx, "test_temp_state1", imasdef.READ_OP
    )
    assert status >= 0, f"Failed to begin first action: status={status}"

    status, data1 = lowlevel.al_read_data(
        op_ctx1, "data1", "", imasdef.INTEGER_DATA, 0
    )
    assert status >= 0, f"Failed to read first data: status={status}"
    assert data1 == 123, f"Expected 123, got {data1}"

    lowlevel.al_end_action(op_ctx1)

    status, op_ctx2 = lowlevel.al_begin_global_action(
        de_ctx, "test_temp_state2", imasdef.READ_OP
    )
    assert status >= 0, f"Failed to begin second action: status={status}"

    status, data2 = lowlevel.al_read_data(
        op_ctx2, "data2", "", imasdef.INTEGER_DATA, 0
    )
    assert status >= 0, f"Failed to read second data: status={status}"
    assert data2 == 456, f"Expected 456, got {data2}"

    lowlevel.al_end_action(op_ctx2)

    lowlevel.al_close_pulse(de_ctx, imasdef.CLOSE_PULSE)


if __name__ == "__main__":
  pytest.main([__file__])

