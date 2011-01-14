/* radare - LGPL - Copyright 2011 pancake<nopcode.org> */

#include <r_fs.h>
#include "grubfs.h"

static RFSFile* FSP(_open)(RFSRoot *root, const char *path) {
	RFSFile *file = r_fs_file_new (root, path);
	GrubFS *gfs = grubfs_new (&FSIPTR, &root->iob);
	file->ptr = gfs;
	file->p = root->p;
	if (gfs->file->fs->open (gfs->file, path)) {
		r_fs_file_free (file);
		grubfs_free (gfs);
		file = NULL;
	} else file->size = gfs->file->size;
	return file;
}

static boolt FSP(_read)(RFSFile *file, ut64 addr, int len) {
	GrubFS *gfs = file->ptr;
	grubfs_bind_io (NULL, file->root->delta);
	gfs->file->fs->read (gfs->file, (char*)file->data, len);
	return R_FALSE;
}

static void FSP(_close)(RFSFile *file) {
	GrubFS *gfs = file->ptr;
	gfs->file->fs->close (gfs->file);
}

static RList *list = NULL;

static int dirhook (const char *filename, const struct grub_dirhook_info *info) {
	RFSFile *fsf = r_fs_file_new (NULL, filename);
	fsf->type = info->dir? 'd':'f';
	fsf->time = info->mtime;
	r_list_append (list, fsf);
	//info->mtimeset
	//info->case_insensitive
	printf ("DIRFILE: %c (%d) %s\n", info->dir?'d':'f',
			info->mtime, filename);
	return 0;
}

static RList *FSP(_dir)(RFSRoot *root, const char *path) {
	GrubFS *gfs = root->ptr;
	list = r_list_new ();
	eprintf ("FSP(_dir: %s\n", path);
	//gfs->file->device->data = &root->iob;
	grubfs_bind_io (&root->iob, root->delta);
	gfs->file->fs->dir (gfs->file->device, path, dirhook);
	grubfs_bind_io (NULL, root->delta);
	return list;
}

static void FSP(_mount)(RFSRoot *root) {
	root->ptr = grubfs_new (&FSIPTR, &root->iob);
}

static void FSP(_umount)(RFSRoot *root) {
	grubfs_free (root->ptr);
	root->ptr = NULL;
}

struct r_fs_plugin_t FSS(r_fs_plugin) = {
	.name = FSNAME,
	.desc = FSDESC,
	.open = FSP(_open),
	.read = FSP(_read),
	.close = FSP(_close),
	.dir = FSP(_dir),
	.mount = FSP(_mount),
	.umount = FSP(_umount),
};