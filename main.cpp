/*
 * nimptool - Nimrod Portal Tool
 * https://github.com/UQ-RCC/nimptool
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 The University of Queensland
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <set>
#include <string>
#include <regex>
#include <filesystem>
#include <charconv>

template<typename F>
static int getgroups(struct passwd *passwd, F&& proc)
{
	assert(passwd != nullptr);

	errno = 0;
	setgrent();
	if(errno != 0)
		return -1;

	struct p { ~p() noexcept { int e = errno; endgrent(); errno = e; } } p;

	struct group grp{}, *g;
	std::vector<char> buf(2048); /* 2048 is enough for the HPC. */

	for(;;)
	{
		int rc = getgrent_r(&grp, buf.data(), buf.size(), &g);
		if(rc == ENOENT)
			break;

		if(rc == ERANGE)
		{
			buf.resize(buf.size() * 2);
			continue;
		}

		if(rc != 0)
		{
			errno = rc;
			break;
		}

		if(g->gr_gid == passwd->pw_gid)
		{
			proc(g);
			continue;
		}

		for(char * const *u = g->gr_mem; *u != nullptr; ++u)
		{
			if(strcmp(passwd->pw_name, *u) == 0)
			{
				proc(g);
				break;
			}
		}
	}

	return errno != 0 ? -1 : 0;
}

static int getdirs(int argc, char **argv)
{
	std::regex regex_qcol("^(Q\\d+)R[W|O]$", std::regex_constants::ECMAScript);

	errno = 0;
	struct passwd *passwd = getpwuid(getuid());
	if(passwd == nullptr)
		return perror("oasswd"), 1;

	std::set<std::string> groups;
	int rc = getgroups(passwd, [&groups, &regex_qcol](const struct group *g) {
		std::string_view group(g->gr_name);

		std::match_results<std::string_view::const_iterator> m;
		if(!std::regex_match(group.begin(), group.end(), m, regex_qcol))
			return;

		groups.emplace(m[1]);
	});

	if(rc < 0)
		return perror("getgroups"), 1;

	/* NB: Is a vector to keep order. */
	std::vector<std::filesystem::path> allowed_paths{
		passwd->pw_dir,
		std::filesystem::path("/30days") / passwd->pw_name,
		std::filesystem::path("/90days") / passwd->pw_name,
	};

	std::filesystem::path qrisdata("/QRISdata");
	for(const std::string& s : groups)
		allowed_paths.push_back(qrisdata / s);

	for(const std::filesystem::path& p : allowed_paths)
		printf("%s\n", p.c_str());
	return 0;
}

static int getacct(int argc, char **argv)
{
	/* Ported from /usr/local/bin/qaccount */
	std::regex regex_acct("^((?:qris-.+|UQ-.+|NCMAS-.+|uqnimrod|sysadmin|support\\d*|training|wheel))$", std::regex_constants::ECMAScript);

	errno = 0;
	struct passwd *passwd = getpwuid(getuid());
	if(passwd == nullptr)
		return perror("oasswd"), 1;

	std::set<std::string> groups;
	int rc = getgroups(passwd, [&groups, &regex_acct](const struct group *g) {
		std::string_view group(g->gr_name);

		std::match_results<std::string_view::const_iterator> m;
		if(!std::regex_match(group.begin(), group.end(), m, regex_acct))
			return;

		groups.emplace(m[1]);
	});

	if(rc < 0)
		return perror("getgroups"), 1;

	for(const std::string& g : groups)
		printf("%s\n", g.c_str());
	return 0;
}

static int checkprocess(int argc, char **argv)
{
	if(argc <= 1)
	{
		fputs("pid,alive\n", stdout);
		return 0;
	}

	bool show_header = true;
	int pidstart = 1;
	if(std::string_view(argv[1]) == "--no-header")
	{
		++pidstart;
		show_header = false;
	}


	for(int i = pidstart; i < argc; ++i)
	{
		std::string_view arg(argv[i]);

		uint16_t pid;
		if(auto [p, ec] = std::from_chars(arg.begin(), arg.end(), pid, 10); ec != std::errc())
			return 2;
	}

	if(chdir("/proc") < 0)
	{
		perror("chdir");
		return 1;
	}

	if(show_header)
		fputs("pid,alive\n", stdout);

	for(int i = pidstart; i < argc; ++i)
	{
		int alive = 0;
		struct stat s{};
		if(::stat(argv[i], &s) >= 0 && ((s.st_mode & S_IFMT) == S_IFDIR))
			alive = 1;

		fprintf(stdout, "%s,%d\n", argv[i], alive);
	}
	return 0;
}

static int checkpidfile(int argc, char **argv)
{
	bool show_header = true;

	const char *filename = nullptr;

	if(argc == 3)
	{
		if(std::string_view(argv[1]) != "--no-header")
			return 2;

		show_header = false;
		filename = argv[2];
	}
	else if(argc != 2)
	{
		return 2;
	}

	if(filename == nullptr)
		filename = argv[1];

	FILE *f = fopen(filename, "rb");
	if(f == nullptr)
	{
		perror("fopen");
		return 1;
	}

	if(chdir("/proc") < 0)
	{
		perror("chdir");
		fclose(f);
		return 1;
	}

	if(show_header)
		fputs("pid,alive\n", stdout);


	unsigned int pid;
	for(int n; (n = fscanf(f, "%u", &pid)) != EOF;)
	{
		if(n == 0)
			break;

		char pidbuf[12];
		snprintf(pidbuf, sizeof(pidbuf), "%u", pid);

		int alive = 0;
		struct stat s{};
		if(::stat(pidbuf, &s) >= 0 && ((s.st_mode & S_IFMT) == S_IFDIR))
			alive = 1;

		fprintf(stdout, "%u,%d\n", pid, alive);
	}

	int err = ferror(f);
	if(err)
		perror("fscanf");

	fclose(f);

	return err != 0;
}

static void print_usage(FILE *f, const char *argv0)
{
	fprintf(f, "Usage: %s checkprocess [--no-header] [pid [pid [pid...]]]\n", argv0);
	fprintf(f, "       %s checkpidfile [--no-header] <pidfile>\n", argv0);
	fprintf(f, "       %s getdirs\n", argv0);
	fprintf(f, "       %s getacct\n", argv0);
}

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		print_usage(stdout, argv[0]);
		return 2;
	}

	int ret = 2;

	std::string_view command(argv[1]);
	if(command == "checkprocess")
		ret = checkprocess(argc - 1, argv + 1);
	else if(command == "checkpidfile")
		ret = checkpidfile(argc - 1, argv + 1);
	else if(command == "getdirs")
		ret = getdirs(argc - 1, argv + 1);
	else if(command == "getacct")
		ret = getacct(argc - 1, argv + 1);

	if(ret != 2)
		return ret;

	print_usage(stdout, argv[0]);
	return 2;
}