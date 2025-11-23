# -*- mode: python ; coding: utf-8 -*-
from PyInstaller.utils.hooks import collect_all

datas = [('compiler.py', '.'), ('settings.py', '.'), ('utils.py', '.'), ('core', 'core'), ('lib', 'lib'), ('libraries', 'libraries'), ('include', 'include'), ('core/runtime/*.h', 'core/runtime'), ('core/runtime/*.c', 'core/runtime'), ('core/runtime/libcyon.a', 'core/runtime'), ('core/runtime/MakeFile', 'core/runtime'), ('libraries/*.c', 'libraries'), ('libraries/*.py', 'libraries'), ('libraries/MakeFile', 'libraries')]
binaries = []
hiddenimports = ['compiler', 'settings', 'utils', 'core.lexer', 'core.parser', 'core.optimizer', 'core.codegen', 'core.interpreter', 'lib.cyonio', 'lib.cyonmath', 'lib.cyonsys', 'backports.functools_lru_cache', 'pkg_resources.py2_warn']
tmp_ret = collect_all('core')
datas += tmp_ret[0]; binaries += tmp_ret[1]; hiddenimports += tmp_ret[2]
tmp_ret = collect_all('lib')
datas += tmp_ret[0]; binaries += tmp_ret[1]; hiddenimports += tmp_ret[2]


a = Analysis(
    ['cli.py'],
    pathex=[],
    binaries=binaries,
    datas=datas,
    hiddenimports=hiddenimports,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    [],
    name='cyon-windows',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=['Pictures\\Cyon.ico'],
)
