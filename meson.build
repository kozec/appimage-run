project('app-installer', 'c')

SOURCES = [
	'src/main.c',
	'src/mount.c',
	'src/imagetools.c',
	'src/xmltools.c',
	'lib/squashfuse_ll.c',
	'lib/squashfuse_ll_inode.c',
	'lib/elf.c',
]

executable('app-installer', SOURCES,
	include_directories: include_directories(
		'lib/include'
	),
	dependencies: [
		dependency('gtk+-3.0', version: '>=3.20'),
		dependency('squashfuse'),
		dependency('libxml-2.0'),
		dependency('fuse'),
		dependency('lzo2'),			# needed by squashfuse but not declared
		dependency('libzstd'),	# needed by squashfuse but not declared
	]
)