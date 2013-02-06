/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "transport/memoryusage.h"

#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#ifndef WIN32
#include <sys/param.h>
#else 
#include <windows.h>
#include <psapi.h>
#endif
#ifdef __MACH__
#include <mach/mach.h>
#elif BSD
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

namespace Transport {

#ifndef WIN32
#ifdef __MACH__

void process_mem_usage(double& vm_usage, double& resident_set, pid_t pid) {

	struct task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

	if (KERN_SUCCESS != task_info(mach_task_self(),
                              TASK_BASIC_INFO, (task_info_t)&t_info, 
                              &t_info_count)) {
		vm_usage = 0;
		resident_set = 0;
    		return;
	}
	vm_usage = t_info.virtual_size;
	resident_set = t_info.resident_size;
}
#elif BSD
void process_mem_usage(double& vm_usage, double& resident_set, pid_t pid) {
	int mib[4];
	size_t size;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	if (pid == 0) {
		mib[3] = getpid();
	}
	else {
		mib[3] = pid;
	}
	struct kinfo_proc proc;

	size = sizeof(struct kinfo_proc);

	if (sysctl((int*)mib, 4, &proc, &size, NULL, 0) == -1) {
		vm_usage = 0;
		resident_set = 0;
		return;
	}

	// sysctl stores 0 in the size if we can't find the process information.
	// Set errno to ESRCH which will be translated in NoSuchProcess later on.
	if (size == 0) {
		vm_usage = 0;
		resident_set = 0;
		return;
	}

	int pagesize,cnt;

	size = sizeof(pagesize);
	sysctlbyname("hw.pagesize", &pagesize, &size, NULL, 0);

	resident_set = (double) (proc.ki_rssize * pagesize / 1024);
	vm_usage = (double) proc.ki_size;
}
#else /* BSD */
void process_mem_usage(double& shared, double& resident_set, pid_t pid) {
	using std::ios_base;
	using std::ifstream;
	using std::string;

	shared       = 0.0;
	resident_set = 0.0;

	// 'file' stat seems to give the most reliable results
	std::string f = "/proc/self/statm";
	if (pid != 0) {
		f = "/proc/" + boost::lexical_cast<std::string>(pid) + "/statm";
	}
	ifstream stat_stream(f.c_str(), ios_base::in);
	if (!stat_stream.is_open()) {
		shared = 0;
		resident_set = 0;
		return;
	}

	// dummy vars for leading entries in stat that we don't care about
	//
	string pid1, comm, state, ppid, pgrp, session, tty_nr;
	string tpgid, flags, minflt, cminflt, majflt, cmajflt;
	string utime, stime, cutime, cstime, priority, nice;
	string O, itrealvalue, starttime;

	// the two fields we want
	//
	unsigned long shared_;
	long rss;

	stat_stream >> O >> rss >> shared_; // don't care about the rest

	long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
	shared     = shared_ * page_size_kb;
	resident_set = rss * page_size_kb;
}
#endif /* else BSD */
#else
#define PSAPI_VERSION 1
#define pid_t void*
	void process_mem_usage(double& shared, double& resident_set, pid_t pid) {
		PROCESS_MEMORY_COUNTERS_EX pmc;
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		shared =  (double)pmc.PrivateUsage;
		resident_set = (double)pmc.WorkingSetSize;
	}
#endif /* WIN32 */

}
