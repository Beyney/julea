# JULEA - Flexible storage framework
# Copyright (C) 2020-2023 Michael Kuhn
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

project('julea', 'c',
	version: '0.5',
	license: 'LGPLv3+',
	meson_version: '>= 0.58',
	default_options: [
		'optimization=g',
		'warning_level=3',
		'c_std=c11',
	]
)

cc = meson.get_compiler('c')
pkg_config = import('pkgconfig')

if get_option('debug')
	args = cc.get_supported_arguments([
		'-fstack-protector-strong',
		'-Waggregate-return',
		'-Wcast-align=strict',
		'-Wcast-qual',
		#'-Wconversion',
		'-Wdeclaration-after-statement',
		'-Wdisabled-optimization',
		'-Wdouble-promotion',
		#'-Wduplicated-branches',
		'-Wduplicated-cond',
		'-Wfloat-equal',
		'-Wformat=2',
		'-Winit-self',
		'-Winline',
		'-Wjump-misses-init',
		'-Wlogical-op',
		'-Wmissing-declarations',
		'-Wmissing-format-attribute',
		'-Wmissing-include-dirs',
		'-Wmissing-noreturn',
		'-Wmissing-prototypes',
		'-Wnested-externs',
		'-Wnull-dereference',
		'-Wold-style-definition',
		'-Wpacked',
		#'-Wpadded',
		'-Wredundant-decls',
		'-Wrestrict',
		'-Wshadow',
		#'-Wsign-conversion',
		'-Wstack-protector',
		'-Wstrict-prototypes',
		'-Wswitch-default',
		'-Wswitch-enum',
		'-Wundef',
		'-Wuninitialized',
		'-Wwrite-strings',
	])

	add_project_arguments(args, language: 'c')
endif

sanitize_cflags = ''

if get_option('b_sanitize') != 'none'
	sanitize_cflags = '-fsanitize=' + get_option('b_sanitize')
endif

# FIXME rpath is not set for dependencies
# FIXME Also check for pkg-config files in /usr (worked around in environment.sh)
# FIXME Allow specifying dependency prefixes?

# g_test_incomplete needs GLib 2.58
# Structured logging needs GLib 2.56
# CentOS 7 has GLib 2.42, CentOS 7.6 has GLib 2.56
# Ubuntu 18.04 has GLib 2.56
glib_version = '2.58'
# Ubuntu 22.04 has libfabric 1.11.0
libfabric_version = '1.11.0'
# Ubuntu 18.04 has libbson 1.9.2
libbson_version = '1.9.0'

hdf_version = '1.14.0'
# Ubuntu 18.04 has LevelDB 1.20
leveldb_version = '1.20'
# Ubuntu 18.04 has LMDB 0.9.21
lmdb_version = '0.9.21'
# Ubuntu 18.04 has libmongoc 1.9.2
libmongoc_version = '1.9.0'
# Ubuntu 18.04 has SQLite 3.22.0
sqlite_version = '3.22.0'
# Ubuntu 18.04 has MariaDB Connector/C 3.0.3
mariadb_version = '3.0.3'
# Ubuntu 18.04 has RocksDB 5.8.8
rocksdb_version = '5.8.8'

# Dependencies

m_dep = cc.find_library('m', required: false)

glib_dep = dependency('glib-2.0',
	version: '>= @0@'.format(glib_version),
	include_type: 'system',
)

gio_dep = dependency('gio-2.0',
	version: '>= @0@'.format(glib_version),
	include_type: 'system',
)

gmodule_dep = dependency('gmodule-2.0',
	version: '>= @0@'.format(glib_version),
	include_type: 'system',
)

gthread_dep = dependency('gthread-2.0',
	version: '>= @0@'.format(glib_version),
	include_type: 'system',
)

gobject_dep = dependency('gobject-2.0',
	version: '>= @0@'.format(glib_version),
	include_type: 'system',
)

libfabric_dep = dependency('libfabric',
	version: '>= @0@'.format(libfabric_version),
	include_type: 'system',
)

libbson_dep = dependency('libbson-1.0',
	version: '>= @0@'.format(libbson_version),
	include_type: 'system',
)

libmongoc_dep = dependency('libmongoc-1.0',
	version: '>= @0@'.format(libmongoc_version),
	required: false,
	include_type: 'system',
)

rados_dep = cc.find_library('rados',
	has_headers: ['rados/librados.h'],
	required: false,
)

# FIXME The config-tool variant does not work with CMake-built HDF5 (see https://github.com/HDFGroup/hdf5/issues/1814)
hdf_dep = dependency('hdf5',
	language: 'c',
	version: '>= @0@'.format(hdf_version),
	required: false,
	method: 'pkg-config',
	include_type: 'system',
)

fuse_dep = dependency('fuse3',
	required: false,
	include_type: 'system',
)

leveldb_dep = dependency('leveldb',
	version: '>= @0@'.format(leveldb_version),
	required: false,
	include_type: 'system',
)

# Ubuntu's package does not ship a pkg-config file
if not leveldb_dep.found()
	leveldb_dep = cc.find_library('leveldb',
		has_headers: ['leveldb/c.h'],
		required: false,
	)
endif

lmdb_dep = dependency('lmdb',
	version: '>= @0@'.format(lmdb_version),
	required: false,
	include_type: 'system',
)

sqlite_dep = dependency('sqlite3',
	version: '>= @0@'.format(sqlite_version),
	required: false,
	include_type: 'system',
)

mariadb_dep = dependency('libmariadb',
	version: '>= @0@'.format(mariadb_version),
	required: false,
	include_type: 'system',
)

# Ubuntu's package does not ship a pkg-config file
if not mariadb_dep.found()
	# FIXME This should use Meson's config-tool
	# FIXME Add some status output
	mariadb_config = find_program('mariadb_config',
		required: false,
	)

	if mariadb_config.found()
		mariadb_cflags = run_command(mariadb_config, '--cflags', check: true).stdout().strip().split()
		mariadb_libs = run_command(mariadb_config, '--libs', check: true).stdout().strip().split()

		mariadb_dep = declare_dependency(
			compile_args: mariadb_cflags,
			link_args: mariadb_libs,
		)
	endif
endif

rocksdb_dep = dependency('rocksdb',
	version: '>= @0@'.format(rocksdb_version),
	required: false,
	include_type: 'system',
)

# Ubuntu's package does not ship a pkg-config file
if not rocksdb_dep.found()
	rocksdb_dep = cc.find_library('rocksdb',
		has_headers: ['rocksdb/c.h'],
		required: false,
	)
endif

otf_dep = dependency('',
	required: false,
)

# FIXME This should use Meson's config-tool
# FIXME Add some status output
otf_config = find_program('otfconfig',
	required: false,
)

if otf_config.found()
	otf_cflags = run_command(otf_config, '--includes', check: true).stdout().strip().split()
	otf_libs = run_command(otf_config, '--libs', check: true).stdout().strip().split()

	otf_dep = declare_dependency(
		compile_args: otf_cflags,
		link_args: otf_libs,
	)
endif

# Compiler checks

stmtim_tvnsec_check = cc.has_member('struct stat', 'st_mtim.tv_nsec',
	args: ['-D_POSIX_C_SOURCE=200809L'],
	prefix: '''
		#include <sys/types.h>
		#include <sys/stat.h>
		#include <unistd.h>
	''',
)

# FIXME has_function is broken for some built-ins
sync_fetch_and_add_check = cc.links('''
	#define _POSIX_C_SOURCE 200809L

	#include <stdint.h>

	int main (void)
	{
		uint64_t dummy = 0;

		__sync_fetch_and_add(&dummy, 1);

		return 0;
	}
''',
	name: '__sync_fetch_and_add'
)

# Configuration

julea_conf = configuration_data()
julea_conf.set('_POSIX_C_SOURCE', '200809L')
julea_conf.set('_FILE_OFFSET_BITS', 64)
julea_conf.set('G_LOG_USE_STRUCTURED', 1)
julea_conf.set_quoted('G_LOG_DOMAIN', 'JULEA')
julea_conf.set_quoted('JULEA_BACKEND_PATH', get_option('prefix') / get_option('libdir') / 'julea' / 'backend')

if get_option('debug')
	julea_conf.set('_FORTIFY_SOURCE', 2)
	julea_conf.set('JULEA_DEBUG', 1)
	julea_conf.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_@0@'.format(glib_version.underscorify()))
	#julea_conf.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_@0@'.format(glib_version.underscorify()))
endif

if hdf_dep.found()
	julea_conf.set('HAVE_HDF5', 1)
endif

if otf_dep.found()
	julea_conf.set('HAVE_OTF', 1)
endif

if stmtim_tvnsec_check
	julea_conf.set('HAVE_STMTIM_TVNSEC', 1)
endif

if sync_fetch_and_add_check
	julea_conf.set('HAVE_SYNC_FETCH_AND_ADD', 1)
endif

configure_file(
	configuration: julea_conf,
	output: 'julea-config.h'
)

# Build

common_deps = [m_dep, glib_dep, gio_dep, gmodule_dep, gthread_dep, gobject_dep, libbson_dep, libfabric_dep, otf_dep]

# FIXME Remove core directory
julea_incs = include_directories([
	'include',
	'include/core',
])

julea_srcs = files([
	'lib/core/distribution/round-robin.c',
	'lib/core/distribution/single-server.c',
	'lib/core/distribution/weighted.c',
	'lib/core/jbackend.c',
	'lib/core/jbackend-operation.c',
	'lib/core/jbackground-operation.c',
	'lib/core/jbatch.c',
	'lib/core/jcache.c',
	'lib/core/jcommon.c',
	'lib/core/jconfiguration.c',
	'lib/core/jconnection-pool.c',
	'lib/core/jcredentials.c',
	'lib/core/jdir-iterator.c',
	'lib/core/jdistribution.c',
	'lib/core/jhelper.c',
	'lib/core/jlist.c',
	'lib/core/jlist-iterator.c',
	'lib/core/jmemory-chunk.c',
	'lib/core/jmessage.c',
	'lib/core/jnetwork.c',
	'lib/core/joperation.c',
	'lib/core/joperation-cache.c',
	'lib/core/jsemantics.c',
	'lib/core/jstatistics.c',
	'lib/core/jtrace.c',
])

julea_lib = shared_library('julea', julea_srcs,
	dependencies: common_deps,
	include_directories: julea_incs,
	c_args: ['-DJULEA_COMPILATION'],
	#soversion: meson.project_version().split('.')[0],
	#version: meson.project_version(),
	install: true,
)

julea_dep = declare_dependency(link_with: julea_lib)

pkg_config.generate(julea_lib,
	description: 'Flexible storage framework',
	extra_cflags: sanitize_cflags,
	subdirs: 'julea',
	requires_private: [glib_dep, gio_dep, gmodule_dep, gthread_dep, gobject_dep, libbson_dep, libfabric_dep],
	url: 'https://github.com/julea-io/julea',
)

julea_client_srcs = {
	'object': files([
		'lib/object/jdistributed-object.c',
		'lib/object/jobject.c',
		'lib/object/jobject-iterator.c',
		'lib/object/jobject-uri.c',
	]),
	'kv': files([
		'lib/kv/jkv.c',
		'lib/kv/jkv-iterator.c',
		'lib/kv/jkv-uri.c',
	]),
	'db-util': files([
		'lib/db-util/jbson.c',
		'lib/db-util/sql-generic-common.c',
		'lib/db-util/sql-generic-tcl.c',
		'lib/db-util/sql-generic-ddl.c',
		'lib/db-util/sql-generic-dml.c',
		'lib/db-util/sql-generic-dql.c'
	]),
	'db': files([
		'lib/db/jdb.c',
		'lib/db/jdb-entry.c',
		'lib/db/jdb-internal.c',
		'lib/db/jdb-iterator.c',
		'lib/db/jdb-schema.c',
		'lib/db/jdb-selector.c',
	]),
	'item': files([
		'lib/item/jcollection.c',
		'lib/item/jcollection-iterator.c',
		'lib/item/jitem.c',
		'lib/item/jitem-iterator.c',
		'lib/item/juri.c',
	])
}

# We need an extra array so that foreach preserves the client order
julea_clients = ['object', 'kv', 'db-util', 'db', 'item']

if hdf_dep.found()
	julea_client_srcs += {
		'hdf5-kv': files([
			'lib/hdf5-kv/jhdf5.c',
		]),
		'hdf5-db': files([
			'lib/hdf5-db/jhdf5-db.c',
			'lib/hdf5-db/jhdf5-db-attr.c',
			'lib/hdf5-db/jhdf5-db-dataset.c',
			'lib/hdf5-db/jhdf5-db-datatype.c',
			'lib/hdf5-db/jhdf5-db-file.c',
			'lib/hdf5-db/jhdf5-db-group.c',
			'lib/hdf5-db/jhdf5-db-link.c',
			'lib/hdf5-db/jhdf5-db-object.c',
			'lib/hdf5-db/jhdf5-db-shared.c',
			'lib/hdf5-db/jhdf5-db-space.c',

		])
	}

	julea_clients += ['hdf5-kv', 'hdf5-db']
endif

julea_client_deps = {}

foreach client: julea_clients
	srcs = julea_client_srcs[client]
	extra_deps = []

	if client == 'item'
		extra_deps += julea_client_deps['object']
		extra_deps += julea_client_deps['kv']
	elif client == 'db'
		extra_deps += julea_client_deps['db-util']
	elif client == 'hdf5-kv'
		extra_deps += julea_client_deps['object']
		extra_deps += julea_client_deps['kv']
		extra_deps += hdf_dep
	elif client == 'hdf5-db'
		extra_deps += julea_client_deps['object']
		extra_deps += julea_client_deps['db']
		extra_deps += hdf_dep
	endif

	julea_client_lib = shared_library('julea-@0@'.format(client), julea_client_srcs[client],
		dependencies: common_deps + [julea_dep] + extra_deps,
		include_directories: julea_incs,
		c_args: ['-DJULEA_@0@_COMPILATION'.format(client.underscorify().to_upper())],
		#soversion: meson.project_version().split('.')[0],
		#version: meson.project_version(),
		install: true,
	)

	julea_client_deps += {client: declare_dependency(link_with: julea_client_lib)}

	if client != 'hdf5-kv' and client != 'hdf5-db'
		pkg_config.generate(julea_client_lib,
			description: 'Flexible storage framework',
			extra_cflags: sanitize_cflags,
			subdirs: 'julea',
			requires: julea_lib,
			url: 'https://github.com/julea-io/julea',
		)
	endif
endforeach

# FIXME name
hdf_deps = []

if hdf_dep.found()
	# FIXME refactor, we do not really want to link to the HDF5 VOL plugins anymore
	hdf_deps += julea_client_deps['hdf5-kv']
	hdf_deps += hdf_dep
endif

julea_test_srcs = files([
	'test/core/background-operation.c',
	'test/core/batch.c',
	'test/core/cache.c',
	'test/core/configuration.c',
	'test/core/credentials.c',
	'test/core/dir-iterator.c',
	'test/core/distribution.c',
	'test/core/list.c',
	'test/core/list-iterator.c',
	'test/core/memory-chunk.c',
	'test/core/message.c',
	'test/core/semantics.c',
	'test/db/db.c',
	'test/hdf5/hdf.c',
	'test/hdf5/hdf-attribute.c',
	'test/hdf5/hdf-dataset.c',
	'test/hdf5/hdf-datatype.c',
	'test/hdf5/hdf-file.c',
	'test/hdf5/hdf-group-link.c',
	'test/hdf5/hdf-helper.c',
	'test/hdf5/hdf-object.c',
	'test/item/collection.c',
	'test/item/collection-iterator.c',
	'test/item/item.c',
	'test/item/item-iterator.c',
	'test/item/uri.c',
	'test/kv/kv.c',
	'test/kv/kv-iterator.c',
	'test/kv/parallel.c',
	'test/object/distributed-object.c',
	'test/object/object.c',
	'test/object/object-iterator.c',
	'test/test.c',
])

julea_test = executable('julea-test', julea_test_srcs,
	dependencies: common_deps + [julea_dep, julea_client_deps['object'], julea_client_deps['kv'], julea_client_deps['db'], julea_client_deps['item']] + hdf_deps,
	include_directories: [julea_incs] + [include_directories('test')],
	install: true,
)

test('julea-test', julea_test,
	# --tap is the default starting with GLib 2.62
	args: ['--tap'],
	protocol: 'tap',
	timeout: 0,
)

julea_benchmark_srcs = files([
	'benchmark/background-operation.c',
	'benchmark/benchmark.c',
	'benchmark/cache.c',
	'benchmark/db/entry.c',
	'benchmark/db/iterator.c',
	'benchmark/db/schema.c',
	'benchmark/hdf5/dai.c',
	'benchmark/hdf5/hdf.c',
	'benchmark/item/collection.c',
	'benchmark/item/item.c',
	'benchmark/kv/kv.c',
	'benchmark/memory-chunk.c',
	'benchmark/message.c',
	'benchmark/object/distributed-object.c',
	'benchmark/object/object.c',
])

executable('julea-benchmark', julea_benchmark_srcs,
	dependencies: common_deps + [julea_dep, julea_client_deps['object'], julea_client_deps['kv'], julea_client_deps['db'], julea_client_deps['item']] + hdf_deps,
	include_directories: [julea_incs] + [include_directories('benchmark')],
	install: true,
)

julea_server_srcs = files([
	'server/loop.c',
	'server/server.c',
])

executable('julea-server', julea_server_srcs,
	dependencies: common_deps + [julea_dep],
	include_directories: julea_incs,
	install: true,
)

julea_backends = [
	'object/gio',
	'object/null',
	'object/posix',
	'kv/null',
	'db/null',
]

if rados_dep.found()
	julea_backends += 'object/rados'
endif

if leveldb_dep.found()
	julea_backends += 'kv/leveldb'
endif

if lmdb_dep.found()
	julea_backends += 'kv/lmdb'
endif

if libmongoc_dep.found()
	julea_backends += 'kv/mongodb'
endif

if rocksdb_dep.found()
	julea_backends += 'kv/rocksdb'
endif

if sqlite_dep.found()
	julea_backends += 'kv/sqlite'
	julea_backends += 'db/sqlite'
endif

if mariadb_dep.found()
	julea_backends += 'db/mysql'
endif

foreach backend: julea_backends
	backend_type = backend.split('/')[0]
	backend_name = backend.split('/')[1]

	extra_args = []
	extra_deps = []

	if backend == 'object/rados'
		extra_deps += rados_dep
	elif backend == 'kv/leveldb'
		extra_deps += leveldb_dep
	elif backend == 'kv/lmdb'
		# lmdb bug
		if meson.get_compiler('c').get_id() == 'clang'
			extra_args += '-Wno-incompatible-pointer-types-discards-qualifiers'
		else
			extra_args += '-Wno-discarded-qualifiers'
		endif
		extra_deps += lmdb_dep
	elif backend == 'kv/mongodb'
		extra_deps += libmongoc_dep
	elif backend == 'kv/rocksdb'
		# https://github.com/facebook/rocksdb/issues/8286
		extra_args += '-Wno-error'
		extra_deps += rocksdb_dep
	elif backend == 'kv/sqlite' or backend == 'db/sqlite'
		extra_deps += sqlite_dep
	elif backend == 'db/mysql'
		# MySQL bug
		if meson.get_compiler('c').get_id() == 'clang'
			extra_args += '-Wno-incompatible-pointer-types-discards-qualifiers'
		else
			extra_args += '-Wno-discarded-qualifiers'
		endif
		extra_deps += mariadb_dep
	endif

	if backend_type == 'db'
		extra_deps += julea_client_deps['db-util']
	endif

	lib = shared_library('@0@-@1@'.format(backend_type, backend_name), files('backend/@0@.c'.format(backend)),
		dependencies: common_deps + [julea_dep] + extra_deps,
		include_directories: julea_incs,
		c_args: extra_args,
		#soversion: meson.project_version().split('.')[0],
		#version: meson.project_version(),
		install: true,
		install_dir: get_option('libdir') / 'julea/backend'
	)
endforeach

julea_cli_srcs = files([
	'cli/cli.c',
	'cli/copy.c',
	'cli/create.c',
	'cli/delete.c',
	'cli/list.c',
	'cli/status.c',
])

executable('julea-cli', julea_cli_srcs,
	dependencies: common_deps + [julea_dep, julea_client_deps['object'], julea_client_deps['kv'], julea_client_deps['item']],
	include_directories: julea_incs,
	install: true,
)

executable('julea-config', 'tools/config.c',
	dependencies: common_deps,
	include_directories: julea_incs,
	install: true,
)

executable('julea-statistics', 'tools/statistics.c',
	dependencies: common_deps + [julea_dep],
	include_directories: julea_incs,
	install: true,
)

if hdf_dep.found()
	executable('julea-h5migrate', 'tools/h5migrate.c',
		dependencies: common_deps + [julea_dep] + [hdf_dep],
		include_directories: julea_incs,
		install: true,
	)
endif

if fuse_dep.found()
	julea_fuse_srcs = files([
		'fuse/access.c',
		'fuse/chmod.c',
		'fuse/chown.c',
		'fuse/create.c',
		'fuse/destroy.c',
		'fuse/getattr.c',
		'fuse/init.c',
		'fuse/julea-fuse.c',
		'fuse/mkdir.c',
		'fuse/read.c',
		'fuse/readdir.c',
		'fuse/rmdir.c',
		'fuse/truncate.c',
		'fuse/unlink.c',
		'fuse/utimens.c',
		'fuse/write.c',
	])

	executable('julea-fuse', julea_fuse_srcs,
		dependencies: [glib_dep, libbson_dep, julea_dep, julea_client_deps['object'], julea_client_deps['kv'], fuse_dep],
		include_directories: julea_incs,
		install: true,
	)
endif

#add_languages('cpp')

#foreach lang: ['c', 'cc']
#	executable('hello-word-@0@'.format(lang), files('example/hello-world.@0@'.format(lang)),
#		dependencies: [glib_dep, libbson_dep, julea_dep, julea_client_deps['object'], julea_client_deps['kv'], julea_client_deps['db']],
#		include_directories: julea_incs,
#	)
#endforeach

# Install

julea_client_hdrs = {
	'core': files([
		'include/core/jbackend.h',
		'include/core/jbackend-operation.h',
		'include/core/jbackground-operation.h',
		'include/core/jbatch.h',
		'include/core/jcache.h',
		'include/core/jconfiguration.h',
		'include/core/jconnection-pool.h',
		'include/core/jcredentials.h',
		'include/core/jdir-iterator.h',
		'include/core/jdistribution.h',
		'include/core/jhelper.h',
		'include/core/jlist.h',
		'include/core/jlist-iterator.h',
		'include/core/jmemory-chunk.h',
		'include/core/jmessage.h',
		'include/core/jnetwork.h',
		'include/core/joperation.h',
		'include/core/jsemantics.h',
		'include/core/jstatistics.h',
		'include/core/jtrace.h',
	]),
	'db': files([
		'include/db/jdb-entry.h',
		'include/db/jdb-error.h',
		'include/db/jdb-iterator.h',
		'include/db/jdb-schema.h',
		'include/db/jdb-selector.h',
		'include/db/jdb-type.h',
	]),
	'item': files([
		'include/item/jcollection.h',
		'include/item/jcollection-iterator.h',
		'include/item/jitem.h',
		'include/item/jitem-iterator.h',
		'include/item/juri.h',
	]),
	'kv': files([
		'include/kv/jkv.h',
		'include/kv/jkv-iterator.h',
		'include/kv/jkv-uri.h',
	]),
	'object': files([
		'include/object/jdistributed-object.h',
		'include/object/jobject.h',
		'include/object/jobject-iterator.h',
		'include/object/jobject-uri.h',
	]),
}

if hdf_dep.found()
	julea_client_hdrs += {
		'hdf5': files([
			'include/hdf5/jhdf5.h',
		]),
	}
endif

foreach client, hdrs: julea_client_hdrs
	install_headers(hdrs,
		subdir: 'julea' / client,
	)
endforeach

julea_hdrs = files([
	'include/julea.h',
	'include/julea-db.h',
	'include/julea-item.h',
	'include/julea-kv.h',
	'include/julea-object.h',
])

if hdf_dep.found()
	julea_hdrs += 'include/julea-hdf5.h'
endif

install_headers(julea_hdrs,
	subdir: 'julea',
)
