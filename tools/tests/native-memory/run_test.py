"""Exercise actual patch code with mock IOKit/mapping; no hardware claims."""
from pathlib import Path
import tempfile,subprocess,shutil
here=Path(__file__).resolve().parent
repo=here.parents[2]
patch=(repo/'xnu/patches/0103-alioth-native-buffer-client.patch').read_text()
with tempfile.TemporaryDirectory(prefix='alioth-native-test-') as temp:
 out=Path(temp);(out/'native-memory').mkdir();(out/'pexpert').mkdir()
 for name in ['SweetNativeMemory.inc','NativeAdrenoABI.h']:
  marker='+++ b/iokit/Kernel/arm/'+name+'\n'
  section=patch.split(marker,1)[1].split('\n--- ',1)[0]
  data=''.join(line[1:]+'\n' for line in section.splitlines() if line.startswith('+'))
  assert data
  (out/'native-memory'/name).write_text(data)
 shutil.copyfile(here/'native_memory_test.cpp',out/'native_memory_test.cpp')
 (out/'pexpert/pexpert.h').write_text('#pragma once\nstatic bool PE_parse_boot_argn(const char*,void*,unsigned){return false;}\n')
 binary=out/'test'
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Wno-unused-function','-fsanitize=address,undefined','-g','-I'+str(out),str(out/'native_memory_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
