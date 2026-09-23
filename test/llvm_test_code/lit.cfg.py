import lit.formats
import os

config.name = "phasar-cli"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.c', '.cpp']

config.test_source_root = os.path.dirname(__file__)

# Only files with a RUN: line are actual lit tests; everything else under
# taint_analysis/ and xtaint/ is not converted yet and would otherwise show
# up as "Unresolved".
config.excludes = ['Output']
for root, _, files in os.walk(config.test_source_root):
  for f in files:
    if not f.endswith(tuple(config.suffixes)):
      continue
    with open(os.path.join(root, f)) as handle:
      if 'RUN:' not in handle.read():
        config.excludes.append(f)

config.test_exec_root = config.phasar_obj_root

config.substitutions.append(
    ('%phasar-cli', os.path.join(config.phasar_cli_dir, 'phasar-cli')))
config.substitutions.append(
    ('%llvm_test_code', config.phasar_ll_dir))
config.substitutions.append(
    ('%config', config.phasar_config_dir))
config.substitutions.append(('%clang', config.clang))
config.substitutions.append(('%clangpp', config.clangpp))

config.environment["PATH"] = os.pathsep.join(
    [config.phasar_filecheck_dir, config.environment["PATH"]]
)
