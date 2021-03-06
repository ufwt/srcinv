# SRCINV implementation v0.6
This doc will show you how to use this framework, how it works,
what it can do, etc.

This document still needs to be perfected.



<span id="0"></span>
### Contents
- [1 - Introduction](#1)
	- [1.1 - The framework](#1.1)
	- [1.2 - The dependencies](#1.2)
	- [1.3 - The target](#1.3)
- [2 - Usage of this framework](#2)
- [3 - Collect information of the target project](#3)
	- [3.1 - gcc-c](#3.1)
	- [3.2 - gcc-asm](#3.2)
- [4 - Parse the data](#4)
	- [4.1 - ADJUST](#4.1)
	- [4.2 - GET BASE](#4.2)
	- [4.3 - GET DETAIL](#4.3)
	- [4.4 - PHASE4](#4.4)
	- [4.5 - PHASE5](#4.5)
	- [4.6 - PHASE6](#4.6)
	- [4.7 - Debug](#4.7)
- [5 - Use the information to do something](#5)
	- [5.1 - show\_detail](#5.1)
- [6 - Future work](#6)
- [7 - References](#7)
- [8 - Greetings](#8)



<span id="1"></span>
### 1 - Introduction
SRCINV is short for source code investigator.

Source code(or binary code) is not only for human who read it, but also for the
compiler(or interpreter or CPU).

This framework aims to do code audit automatically for open source projects.

For QA/researchers, they could find different kinds of bugs in their projects.

This framework may also automatically generate samples to test the bug and
patches for the bug.

[Back to Contents](#0)



<span id="1.1"></span>
### 1.1 - The framework
- **Directories**
	- analysis
	> parse resfile(s), and provide some helper functions for hacking to
	> call

	- bin
	> srcinv binaries and modules

	- collect
	> As a compiler plugin, run in target project root directory, collect
	> the compiling data(AST)

	- config
	> runtime configs. Currently in-use file is module.conf

	- core
	> the main process, handle user input, parse configs, load modules, etc.

	- doc
	> Documatations, ChangeLog

	- hacking
	> do anything you want to do. For example, locations that field gid of
	> struct cred get used, list functions in directory or a single souce
	> file, etc.

	- include
	> header files

	- lib
	> provide some compiler functions to use in analysis.

	- testcase
	> some cases that should be handled in analysis.

	- tmp
	> result data: resfile(s), src.saved, log, etc

[Back to Contents](#0)



<span id="1.2"></span>
### 1.2 - The dependencies
The framework could only run on 64bit GNU/Linux systems, and need personality
system call to disable the current process aslr.

The colorful prompt could be incompatible.

Libraries:
- [clib](https://github.com/snorez/clib/)
- ncurses
- readline
- libcapstone

Header files:
- gcc plugin library

[Back to Contents](#0)



<span id="1.3"></span>
### 1.3 - The targets
For now, the framework is only for C projects compiled by GCC.

[Back to Contents](#0)



<span id="2"></span>
### 2 - Usage of this framework
This framework has several levels:
- level 0: SRCINV(the main).
- level 1: collect, analysis, hacking.

Commands for level 0:
```c
SRCINV> help
================== USAGE INFO ==================
help
	Show this help message
quit
	Exit this main process
exit
	Exit this main process
do_sh
	Execute bash command
showlog
	Show current log messages
reload_config
	Reload config
load_srcfile
	(srcid)
flush_srcfile
	Write current src info to srcfile
collect
	Enter collect subshell
analysis
	Enter analysis subshell
hacking
	Enter hacking subshell
================== USAGE END ==================
```

Commands for level 1 - collect:
```c
<1> collect> help
================== USAGE INFO ==================
help
	Show this help message
exit
	Return to the previous shell
quit
	Return to the previous shell
show
	(type) [path]
	type: (SRC/BIN)_(KERN/USER)_(LINUX/...)_(...) check si_core.h for more
	Pick appropriate module and show comment
================== USAGE END ==================
```

Commands for level 1 - analysis:
```c
<1> analysis> help
================== USAGE INFO ==================
help
	Show this help message
exit
	Return to the previous shell
quit
	Return to the previous shell
parse
	(resfile) (kernel) (builtin) (step) (auto_Y)
	Get information of resfile, steps are:
		0 Get all information
		1 Get information adjusted
		2 Get base information
		3 Get detail information
		4 Prepare for step5
		5 Get indirect call information
		6 Check if all GIMPLE_CALL are set
getoffs
	(resfile) (filecnt)
	Count filecnt files and calculate the offset
cmdline
	(resfile) (filepath)
	Show the command line used to compile the file
================== USAGE END ==================
```

Commands for level 1 - hacking:
```c
================== USAGE INFO ==================
help
	Show this help message
exit
	Return to the previous shell
quit
	Return to the previous shell
itersn
	[output_path]
	Traversal all sinodes to stderr/file
havefun
	Run hacking modules
load_sibuf
	(sibuf_addr)
	Loading specific sibuf
list_function
	[dir/file] (format)
	List functions in dir or file
	format:
		0(normal output)
		1(markdown output)
================== USAGE END ==================
```

This framework take a few steps to parse the project: collect the information,
parse the collected data, use the parsed data.

[Back to Contents](#0)



<span id="3"></span>
### 3 - Collect information of the target project
**NOTE: before collecting information of a project, make sure the resfile not
exist yet.**

When compiling a source file, GCC will handle several data formats: AST, GIMPLE,
etc. This framework collects the lower GIMPLEs right at `ALL_IPA_PASSES_END`,
researchers could use these information to parse the project.

Generally, a GCC plugin could only see the information of the current compiling
file. When we put the information of all compiled files together, we could see
the big picture of the project, make it more convenience to parse the project.

For each project, this framework use a src structure to track all information,
which could be:
- sibuf: a list of compiled files
- resfile: a list of all the resfiles for the target project.
> For linux kernel, there could be one vmlinux-resfile which is builtin and
> a bunch of module-resfiles which are non-builtin.
- sinodes: nodes for non-local variables, functions, data types.
> These nodes could be searched by name or by location.
>
> For static file-scope variables, static file-scope functions,
> data types have a location, we use location to search and insert.
>
> For global variables, global functions, and data types with a name but
> without a location, we use name to search and insert.
>
> For data types without location or name, we put them into sibuf.

About GCC plugin, this framework gets the lower GIMPLEs.
```c
source file ---> AST ---> High-level GIMPLE ---> Low-level GIMPLE
```

We collect the GIMPLEs at ALL\_IPA\_PASSES\_END.
```c
all_lowering_passes
|--->	useless		[GIMPLE_PASS]
|--->	mudflap1	[GIMPLE_PASS]
|--->	omplower	[GIMPLE_PASS]
|--->	lower		[GIMPLE_PASS]
|--->	ehopt		[GIMPLE_PASS]
|--->	eh		[GIMPLE_PASS]
|--->	cfg		[GIMPLE_PASS]
...
all_ipa_passes
|--->	visibility	[SIMPLE_IPA_PASS]
...
```

Each source file will have only one chance to call the plugin callback function
at ALL\_IPA\_PASSES\_END. It means that the data of a function we collected
before would not be modified when we collecting some other function's data.

For more about GCC plugin, check [refs[1]](#ref1) or [refs[2]](#ref2).

For C projects, `collect/c.cc` is used when compile the target project. It is
not used in `si_core` process. It is a GCC plugin. When this plugin gets
involved, it detects the current compling file's name and full path, register
a callback function for PLUGIN\_ALL\_IPA\_PASSES\_START event. The resfile is
PAGE\_SIZE aligned.

We can't register PLUGIN\_PRE\_GENERICIZE to handle each tree\_function\_decl,
cause the function may be incompleted.
```c
static void test_func0(void);
static void test_func1(void)
{
	test_func0();
}

static void test_func0(void)
{
	/* test_func0 body */
}
```
In this case, when PLUGIN\_PRE\_GENERICIZE event happens, the function
test\_func1 is handled. However, the tree\_function\_decl for test\_func0 is
not initialized yet.

When the registered callback function gets called, the function body would be
moved to `tree_function_decl->f->cfg` from `tree_function_decl->saved_tree`:
```c
tree_function_decl->saved_tree --->
tree_function_decl->f->gimple_body --->
tree_function_decl->f->cfg
```

When collect data of a var, we focus on non-local variables. GCC provides a
function to test if this is a global var.
```c
static inline bool is_global_var (const_tree t)
{
	return (TREE_STATIC(t) || DECL_EXTERNAL(t));
}
```
for VAR\_DECL, TREE\_STATIC check whether this var has a static storage or not.
DECL\_EXTERNAL check if this var is defined elsewhere.
We add an extra check:
```c
if (is_global_var(node) &&
	((!DECL_CONTEXT(node)) ||
	 (TREE_CODE(DECL_CONTEXT(node)) == TRANSLATION_UNIT_DECL))) {
		objs[start].is_global_var = 1;
	}
```
DECL\_CONTEXT is NULL or the TREE\_CODE of it is TRANSLATION\_UNIT\_DECL, we
take this var as a non-local variable.

The test is for ubuntu 18.04 linux-5.3.y, gcc 8.3.0. The size of the data we
collect is about 30G.

[Back to Contents](#0)



<span id="3.1"></span>
### 3.1 - gcc-c
Check [Chapter 3](#3)

[Back to Contents](#0)



<span id="3.2"></span>
### 3.2 - gcc-asm
Please add new `si_type` in `include/si_core.h`, do not forget to modify
`config/module.conf` and `si_module.c`.

For linux kernel, `EXTRA_AFLAGS` is needed while run `make`.

Note that, we can not set a `file_content.si_type.*` to BOTH/ANY value, the
BOTH/ANY values are for modules in collect/analysis.

[Back to Contents](#0)



<span id="4"></span>
### 4 - Parse the data
The main feature of the framework is parsing the data. However, it is
the most complicated part.

This is implemented in analysis/analysis.c and analysis/gcc/c.cc(for GCC c
project).

For a better experience, the framework use ncurses to show the parse status
(check the main Makefile).

To parse the data, we need to load the resfile into memory.
However, the resfile for linux-5.3.y is about 30G. We can not load it all.
Here comes the solution:
- Disable aslr and restart the process.
- Use mmap to load resfile at RESFILE\_BUF\_START, if the resfile current
loaded in memory is larger than RESFILE_BUF_SIZE, unmap the last, and do the
next mmap just at the end of the last memory area.
- For each compiled file, we use a sibuf to track the address where to load
the data, the size, and the offset of the resfile.
- Use `resfile__resfile_load()` to load a compiled file's information.

```c
memory layout:
0x0000000000000000	0x0000000000400000	NULL pages
0x0000000000400000	0x0000000000403000	si_core .text
0x0000000000602000	0x0000000000603000	si_core .rodata ...
0x0000000000603000	0x0000000000605000	si_core .data ...
0x0000000000605000	0x0000000000647000	heap
SRC_BUF_START     	RESFILE_BUF_START 	srcfile
RESFILE_BUF_START 	0x????????????????	resfiles
0x0000700000000000	0x00007fffffffffff	threads libs plugins stack...
```

If SRC\_BUF\_START is 0x100000000, RESFILE\_BUF\_START is 0x1000000000,
the size of src memory area is up to 64G, the size of resfile could be 1024G
or larger.

The size of the src.saved after PHASE6 is 4.7G. It takes about 6 hours to do
all the six phases.

[Back to Contents](#0)



<span id="4.1"></span>
### 4.1 - ADJUST
The data we collect, a lot of pointers in it. We must adjust these pointers
before we read it, locations as well.

Note that, location\_t in GCC is 4 bytes, so we set it an offset value.
use `*(expanded_location *)(sibuf->payload + loc_value)` to get the location.

[Back to Contents](#0)



<span id="4.2"></span>
### 4.2 - GET BASE
Base info include:
- TYPE\_FUNC\_GLOBAL,
- TYPE\_FUNC\_STATIC,
- TYPE\_VAR\_GLOBAL,
- TYPE\_VAR\_STATIC,
- TYPE\_TYPE,
- TYPE\_FILE

Check if the location or name exists in sinodes. If not, alloc a new sinode.
If exists, and searched by name, should check if the name conflict(weak
symbols?).

[Back to Contents](#0)



<span id="4.3"></span>
### 4.3 - GET DETAIL
For TYPE\_TYPE, need to know the type it points to, or the size, or the fields.

For TYPE\_VAR\_\*, just get the type of it.

For function, get the type of return value, the arguments list, and the
function body.

[Back to Contents](#0)



<span id="4.4"></span>
### 4.4 - PHASE4
PHASE4, set possible\_list for each non-local variables, get variables and
functions(not direct call) marked(use\_at\_list).

[Back to Contents](#0)



<span id="4.5"></span>
### 4.5 - PHASE5
PHASE5, handle marked functions, then try best to trace calls.

A direct call is the second op of GIMPLE_CALL statement is addr of a function.
If the second op of GIMPLE\_CALL is VAR\_DECL or PARM\_DECL, it is an indirect
call.

NOTE, we are dealing with tree\_ssa\_name now.

[Back to Contents](#0)



<span id="4.6"></span>
### 4.6 - PHASE6
Nothing to do here, do phase5 again.

[Back to Contents](#0)



<span id="4.7"></span>
### 4.7 - Debug
This section will discuss some efficient steps help to locate bugs.

Do the STEP one by one, and backup the resfile/src.saved files to resfile.x/src.saved.x(x is the step number). So we can restore them while the parsing crashed.

I suggest you to backup resfile/src.saved after STEP1 and STEP3.

If `HAVE_CLIB_DBG_FUNC` is enabled, the stack trace message shows which thread
has crashed. With this, we can find the source file the thread is parsing.

Then, edit `Makefile`, use `O0`, and recompile.

In PARSE mode, we provide a command `one_sibuf`, it is used to parse only one
source file. Thus, we can do `one_sibuf /path/to/the/file STEP 1` to
reproduce the crash.

[Back to Contents](#0)



<span id="5"></span>
### 5 - Use the information to do something
We need to write some plugins to use parsed data. As we collect the lower
GIMPLEs, these are what we handle in hacking plugins.

For example(obsolete):
```c
static void test_func(int flag)
{
	int need_free;
	char *buf;
	if (flag) {
		buf = (char *)malloc(0x10);
		need_free = 1;
	}

	/* do something here */

	if (need_free)
		free(buf);
}
```
plugins/uninit.cc shows how to detect this kind of bugs.

This plugin detects all functions one by one:
- generate all possible code_path of this function
- traversal all local variables(not static)
	- traversal code_paths, find first position this variable used
	- check if the first used statement is to read this variable. [The demo](https://www.youtube.com/watch?v=anNoHjrYqVc)

[Back to Contents](#0)



<span id="5.1"></span>
### 5.1 - show_detail
show\_detail: Show detail of variables, functions, types.

For data types, sometimes, we want to know where does the field of a structure
get used at. `show_detail xxx.yyy.zzz type [all|src|used|offset|size]` should
output the message.

We can do more. If a structure contains some fields without a name, we use `*`.
```c
struct xxx {
	struct list_head sibling;
	union {
		struct hlist_node list0;
		struct hlist_node list1;
	};
	/* ... */
}
```
`show_detail xxx.*.list0 type used` will show the used-at results.

Some used\_at locations still can not be found.
> Example, hlist\_for\_each\_entry\_safe(x,n,head,xxlist):
> the xxlist can not be found uses here because it is not used directly,
> the xxlist.next is used instead.
>
> In analysis/gcc/c.cc, `__4_mark_gimple_op()`->`__4_mark_component_ref(op)`,
> the first operand of op would be a COMPONENT\_REF while the second one is
> a FIELD\_DECL(next in hlist\_node). Thus, the first COMPONENT\_REF is xxlist
> which is a hlist\_node, we don't get that use here!

Test result for clib
```c
<1> hacking> show_detail clib_mm.* type
src: /home/zerons/workspace/clib/include/clib_mm.h 43 19
Used at(sibling):
	clib_mm_find 26 2 0x10075b7bbc 1
	clib_mm_setup 187 2 0x10075905cc 1
Offset:
	0
Size:
	128
Used at(desc):
	clib_mm_find 27 8 0x10075b7154 1
	clib_mm_setup 178 10 0x1007594088 1
Offset:
	128
Size:
	64
Used at(fd):
	clib_mm_init 51 8 0x10075b076f 0
	clib_mm_dump 63 7 0x10075ae833 1
	clib_mm_expand 120 9 0x10075a87a0 1
Offset:
	192
Size:
	32
Used at(refcount):
	clib_mm_find 28 15 0x10075b6604 1
	clib_mm_put 142 26 0x100759f5de 1
Offset:
	256
Size:
	64
Used at(mm_start):
	clib_mm_init 52 14 0x10075b0837 0
	clib_mm_expand 103 33 0x10075abb2c 1
	clib_mm_expand 114 51 0x10075a7ce0 1
	clib_mm_expand 120 54 0x10075a8888 1
	clib_mm_cleanup 214 50 0x100757fad2 1
Offset:
	320
Size:
	64
Used at(mm_head):
	clib_mm_init 53 13 0x10075b08ff 0
	clib_mm_dump 65 58 0x10075ae043 1
Offset:
	384
Size:
	64
Used at(mm_cur):
	clib_mm_init 54 12 0x10075b09c7 0
	clib_mm_dump 65 46 0x10075adfd3 1
	clib_mm_expand 76 21 0x10075a5042 1
	clib_mm_get 246 16 0x1007572c43 1
	clib_mm_get 247 12 0x1007572de3 0
Offset:
	448
Size:
	64
Used at(mm_tail):
	clib_mm_init 55 13 0x10075b0b07 0
	clib_mm_expand 76 8 0x10075a53ca 1
	clib_mm_expand 94 14 0x10075a4792 1
	clib_mm_expand 94 14 0x10075a648a 0
	clib_mm_expand 114 38 0x10075a7d50 1
	clib_mm_cleanup 214 37 0x1007580582 1
Offset:
	512
Size:
	64
Used at(mm_end):
	clib_mm_init 56 12 0x10075b0c47 0
	clib_mm_expand 82 8 0x10075a4ce2 1
	clib_mm_expand 120 44 0x10075a88f8 1
Offset:
	576
Size:
	64
Used at(expandable):
	clib_mm_init 57 16 0x10075b0d87 0
	clib_mm_expand 97 7 0x10075ac054 1
	clib_mm_expand 121 7 0x10075a89d8 1
Offset:
	640
Size:
	8
```

[Back to Contents](#0)



<span id="6"></span>
### 6 - Future work
Check [TODO.md](https://github.com/hardenedlinux/srcinv/blob/master/doc/TODO.md)

[Back to Contents](#0)



<span id="7"></span>
### 7 - References
[0] [srcinv project homepage](https://github.com/hardenedlinux/srcinv/)

<span id="ref1"></span>
[1] [GNU Compiler Collection Internals](https://gcc.gnu.org/onlinedocs/gcc-8.3.0/gccint.pdf)

<span id="ref2"></span>
[2] [GCC source code](http://mirrors.concertpass.com/gcc/releases/gcc-8.3.0/gcc-8.3.0.tar.gz)

[3] [深入分析GCC](https://www.amazon.cn/dp/B06XCPZFKD)

[4] [gcc plugins for linux kernel](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/scripts/gcc-plugins?h=v4.14.85)

[Back to Contents](#0)


<span id="8"></span>
### 8 - Greetings
Thanks to the author of <<深入分析GCC>> for helping me to understand the
inside of GCC.

Thanks to PYQ and other workmates, with your understanding and support, I can
focus on this framework.

Thanks to CG for his support during the development of the framework.

Certainly... Thanks to all the people who managed to read the whole text.

There is still a lot of work to be done. Ideas or contributions are always
welcome. Feel free to send push requests.

[Back to Contents](#0)
