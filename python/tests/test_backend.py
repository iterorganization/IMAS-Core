import os
import pathlib

import pytest

import imas_core


def _create_temp_ids_file(
    content: str, suffix: str, tmp_path: pathlib.Path
) -> str:
  filename = os.path.join(tmp_path, f"test_temp{suffix}.ids")
  with open(filename, "w") as f:
    f.write(content)
  return filename


def test_state_persistence(tmp_path: pathlib.Path):
  """Test that reading two IDS files in sequence correctly clears state."""

  content1 = "test_temp_state1/data1\n\ttype: 51 (integer)\n\tdim: 0\n123\n"
  _create_temp_ids_file(content1, "_state1", tmp_path)

  content2 = "test_temp_state2/data2\n\ttype: 51 (integer)\n\tdim: 0\n456\n"
  _create_temp_ids_file(content2, "_state2", tmp_path)

  uri = f"imas:ascii?path={tmp_path}"

  _, de_ctx = imas_core.lowlevel.al_begin_dataentry_action(
      uri, imas_core.imasdef.OPEN_PULSE
  )

  _, op_ctx1 = imas_core.lowlevel.al_begin_global_action(
      de_ctx, "test_temp_state1", imas_core.imasdef.READ_OP
  )

  _, data1 = imas_core.lowlevel.al_read_data(
      op_ctx1, "data1", "", imas_core.imasdef.INTEGER_DATA, 0
  )
  assert data1 == 123, f"Expected 123, got {data1}"

  imas_core.lowlevel.al_end_action(op_ctx1)

  _, op_ctx2 = imas_core.lowlevel.al_begin_global_action(
      de_ctx, "test_temp_state2", imas_core.imasdef.READ_OP
  )

  _, data2 = imas_core.lowlevel.al_read_data(
      op_ctx2, "data2", "", imas_core.imasdef.INTEGER_DATA, 0
  )
  assert data2 == 456, f"Expected 456, got {data2}"

  imas_core.lowlevel.al_end_action(op_ctx2)

  imas_core.lowlevel.al_close_pulse(de_ctx, imas_core.imasdef.CLOSE_PULSE)


if __name__ == "__main__":
  pytest.main([__file__])
