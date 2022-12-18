#define FUSE_USE_VERSION 29
#define SHA_LENGTH 64

#define EVP_ERROR(context) { EVP_MD_CTX_free(context); return -ENOMEM; }

#include <openssl/evp.h>
#include <fuse.h>

#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

string root;

string realPath(const char *path) {
	return root + path;
}

string realPath(const char *path, const string &subpath) {
	return root + path + '/' + subpath;
}

int sha_getattr(const char *path, struct stat *stat) {
	int result = ::stat(realPath(path).c_str(), stat);
	if (!result && S_ISREG(stat->st_mode))
		stat->st_size = SHA_LENGTH;
	return result;
}

int sha_readdir(const char *path, void *buff, fuse_fill_dir_t filler, off_t, struct fuse_file_info *) {
	DIR *dir = opendir(realPath(path).c_str());
	if (!dir)
		return -ENOENT;
	struct dirent *dirent;
	while ((dirent = readdir(dir))) {
		string name(dirent->d_name);
		struct stat stat;
		if (::stat(realPath(path, name).c_str(), &stat))
			continue;
		if (S_ISREG(stat.st_mode) || S_ISDIR(stat.st_mode))
			filler(buff, name.c_str(), NULL, 0);
	}
	return 0;
}

int sha_open(const char *path, struct fuse_file_info *) {
	struct stat stat;
	if (::stat(realPath(path).c_str(), &stat) || !S_ISREG(stat.st_mode))
		return -ENOENT;
	return 0;
}

int sha_release(const char *path, struct fuse_file_info *) {
	return 0;
}

int sha_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *) {
	ifstream ifs(realPath(path).c_str());
	if (!ifs)
		return -ENOENT;
	if (offset >= SHA_LENGTH)
		return 0;
	EVP_MD_CTX* context = EVP_MD_CTX_new();
	if (!EVP_DigestInit_ex(context, EVP_sha256(), NULL))
		return -ENOMEM;
	char rbuff[BUFSIZ];
	while (!ifs.eof()) {
		ifs.read(rbuff, BUFSIZ);
		if (!EVP_DigestUpdate(context, rbuff, ifs.gcount()))
			EVP_ERROR(context);
	}
	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int length;
	if (!EVP_DigestFinal_ex(context, hash, &length))
		EVP_ERROR(context);
	stringstream ss;
	for (unsigned int i = 0; i < length; i++) {
		int current = hash[i];
		if (current < 10)
			ss << '0';
		ss << hex << (int) hash[i];
	}
	string output(ss.str());
	size_t osize = output.size() - offset;
	if (size < osize)
		osize = size;
	memcpy(buff, output.data(), osize);
	return osize;
}

static struct fuse_operations operations;

int main(int argc, char** argv) {
	if (argc != 3) {
		cerr << "Usage:" << endl;
		cerr << argv[0] << " root mount" << endl;
		return 1;
	}
	// we don't use designated initializer due to old g++ in Ubuntu 18
	operations.getattr = sha_getattr;
	operations.readdir = sha_readdir;
	operations.open = sha_open;
	operations.read = sha_read;
	operations.release = sha_release;
	root = argv[1];
	argv[1] = argv[2];
	argv[2] = NULL;
	return fuse_main(argc - 1, argv, &operations, NULL);
}

// make g++ libssl-dev libfuse-dev CONF=ubuntu
