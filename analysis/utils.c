/*
 * TODO
 * Copyright (C) 2018  zerons
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define	vecpfx m_vecpfx
#define	vecdata m_vecdata
#include "si_core.h"

/*
 * ************************************************************************
 * get_func_code_paths_start get_func_next_code_path
 * get code_paths in a function
 * ************************************************************************
 */
static __thread struct code_path *code_path_labels[LABEL_MAX];
static __thread size_t code_path_label_idx = 0;
static __thread size_t code_path_idx = 0;
static int code_path_label_checked(struct code_path *codes)
{
	for (size_t i = 0; i < code_path_label_idx; i++) {
		if (code_path_labels[i] == codes)
			return 1;
	}
	return 0;
}

static void code_path_label_insert(struct code_path *codes)
{
	code_path_labels[code_path_label_idx++] = codes;
}

static void get_code_path_labels(struct code_path *codes)
{
	if (!codes)
		return;
	if (!code_path_label_checked(codes))
		code_path_label_insert(codes);

	for (size_t i = 0; i < codes->branches; i++) {
		if (!code_path_label_checked(codes->next[i]))
			get_code_path_labels(codes->next[i]);
	}
}

void get_func_code_paths_start(struct code_path *codes)
{
	for (size_t i = 0; i < code_path_label_idx; i++) {
		code_path_labels[i] = NULL;
	}
	code_path_label_idx = 0;
	code_path_idx = 0;

	get_code_path_labels(codes);
}

struct code_path *get_func_next_code_path(void)
{
	if (code_path_idx == code_path_label_idx)
		return NULL;
	struct code_path *ret = code_path_labels[code_path_idx];
	code_path_idx++;
	return ret;
}

/*
 * ************************************************************************
 * trace_var
 * ************************************************************************
 */
/* -1: error, 0: found, 1: var is from user */
int trace_var(struct sinode *fsn, void *var_parm,
		struct sinode **target_fsn, void **target_node)
{
#if 0
	int ret = 0;
	tree node = (tree)var_parm;
	struct func_node *fn = (struct func_node *)fsn->data;

	enum tree_code tc = TREE_CODE(node);
	switch (tc) {
	case VAR_DECL:
	{
		/* TODO */
		break;
	}
	case PARM_DECL:
	{
		int parm_idx = 0;
		int found = 0;
		struct var_list *tmp;
		list_for_each_entry(tmp, &fn->args, sibling) {
			parm_idx++;
			if (tmp->var.node == node) {
				found = 1;
				break;
			}
		}
		BUG_ON(!found);

		struct callf_list *caller;
		list_for_each_entry(caller, &fn->callers, sibling) {
			if (caller->call_id.id1 == fsn->node_id.id.id1)
				continue;

			BUG_ON(caller->value_flag);
			BUG_ON(caller->body_missing);
			BUG_ON(siid_type(&caller->call_id) == TYPE_FILE);

			struct sinode *caller_sn;
			union siid *tid = (union siid *)&caller->value;
			caller_sn = analysis__sinode_search(siid_type(tid),
								SEARCH_BY_ID, tid);
			BUG_ON(!caller_sn);

			resfile__resfile_load(caller_sn->buf);
			struct func_node *caller_fn =
						(struct func_node *)caller_sn->data;
			struct callf_list *caller_target_cfl;
			caller_target_cfl = callf_list_find(&caller_fn->callees,
							fsn->node_id.id.id1);
			BUG_ON(!caller_target_cfl);

			struct callf_stmt_list *call_gs_list;
			list_for_each_entry(call_gs_list,
						&caller_target_cfl->gimple_stmts,
						sibling) {
				gimple_seq gs = (gimple_seq)call_gs_list->gimple_stmt;
				tree *ops = gimple_ops(gs);
				BUG_ON((parm_idx+1) >= gimple_num_ops(gs));
				tree parm_op = ops[parm_idx+1];

				*target_fsn = caller_sn;
				*target_node = parm_op;
			}
		}
		break;
	}
	default:
	{
		fprintf(stderr, "miss %s\n", tree_code_name[tc]);
		BUG();
	}
	}
#endif
	return 0;
}

/*
 * ************************************************************************
 * generate functions' call level
 * if this is kernel, sys_* is level 0
 * if this is user program, main is level 0, thread routine is level 0
 * if this is user library, all global functions are level 0
 * ************************************************************************
 */

/*
 * ************************************************************************
 * generate paths from `fsn_from` to `fsn_to`
 * gen_func_paths: for functions, not code_paths
 * gen_code_paths: for code paths
 * ************************************************************************
 */
static __thread void *path_funcs[CALL_LEVEL_DEEP];
static __thread size_t path_func_deep;
static void push_func_path(struct sinode *fsn)
{
	path_funcs[path_func_deep] = (void *)fsn;
	path_func_deep++;
}

static void pop_func_path(void)
{
	path_func_deep--;
}

static int check_func_path(struct sinode *fsn)
{
	for (size_t i = 0; i < path_func_deep; i++) {
		if (path_funcs[i] == (void *)fsn)
			return 1;
	}
	return 0;
}

static void create_func_paths(struct list_head *head)
{
	struct path_list *new_head;
	new_head = path_list_new();
	for (size_t i = 0; i < path_func_deep; i++) {
		struct funcp_list *_new;
		_new = funcp_list_new();
		_new->fsn = (struct sinode *)path_funcs[i];
		list_add_tail(&_new->sibling, &new_head->path_head);
	}
	list_add_tail(&new_head->sibling, head);
}

void gen_func_paths(struct sinode *fsn_from, struct sinode *fsn_to,
			struct list_head *head, int idx)
{
	if (!idx) {
		path_func_deep = 0;
		INIT_LIST_HEAD(head);
	}

	push_func_path(fsn_from);
	if (fsn_from == fsn_to) {
		create_func_paths(head);
		pop_func_path();
		return;
	}

	struct func_node *fn;
	fn = (struct func_node *)fsn_from->data;

	struct callf_list *tmp;
	list_for_each_entry(tmp, &fn->callees, sibling) {
		if (tmp->value_flag)
			continue;
		if (tmp->body_missing)
			continue;
		union siid *tid = (union siid *)&tmp->value;
		struct sinode *next_fsn;
		next_fsn = analysis__sinode_search(siid_type(tid),
							SEARCH_BY_ID, tid);
		BUG_ON(!next_fsn);

		if (check_func_path(next_fsn))
			continue;

		gen_func_paths(next_fsn, fsn_to, head, idx+1);
	}

	pop_func_path();
	return;
}

void drop_func_paths(struct list_head *head)
{
	struct path_list *tmp0, *next0;
	list_for_each_entry_safe(tmp0, next0, head, sibling) {
		struct funcp_list *tmp1, *next1;
		list_for_each_entry_safe(tmp1, next1, &tmp0->path_head, sibling) {
			list_del(&tmp1->sibling);
			free(tmp1);
		}
		list_del(&tmp0->sibling);
		free(tmp0);
	}
}

/*
 * ************************************************************************
 * generate code paths for function fsn
 * ************************************************************************
 */
static __thread void *func_code_paths[LABEL_MAX];
static __thread size_t func_code_path_deep;
static void push_func_codepath(struct code_path *cp)
{
	func_code_paths[func_code_path_deep] = (void *)cp;
	func_code_path_deep++;
}

static void pop_func_codepath(void)
{
	func_code_path_deep--;
}

static int check_func_code_path(struct code_path *cp)
{
	for (size_t i = 0; i < func_code_path_deep; i++) {
		if (func_code_paths[i] == (void *)cp)
			return 1;
	}
	return 0;
}

static __thread struct timeval tv_start, tv_cur;
#define	TIME_FOR_ONE_CP	0x10
static void create_func_code_paths(struct clib_rw_pool *pool)
{
	struct path_list *new_head;
	new_head = path_list_new();
	for (size_t i = 0; i < func_code_path_deep; i++) {
		struct codep_list *_new;
		_new = codep_list_new();
		_new->cp = (struct code_path *)func_code_paths[i];
		list_add_tail(&_new->sibling, &new_head->path_head);
	}
	clib_rw_pool_push(pool, (void *)new_head);
	gettimeofday(&tv_start, NULL);
}

static int code_path_last(struct code_path *cp)
{
	for (size_t i = 0; i < cp->branches; i++) {
		if (cp->next[i])
			return 0;
	}
	return 1;
}

static void gen_func_codepaths(struct code_path *cp, struct clib_rw_pool *pool,
				int idx, int flag, atomic_t *paths)
{
	if (!idx) {
		func_code_path_deep = 0;
		atomic_set(paths, 0);
		gettimeofday(&tv_start, NULL);
	}

	gettimeofday(&tv_cur, NULL);
	if ((tv_cur.tv_sec - tv_start.tv_sec) > TIME_FOR_ONE_CP) {
		atomic_set(paths, FUNC_CP_MAX + 1);
		return;
	}

	push_func_codepath(cp);
	if (code_path_last(cp)) {
		if (atomic_read(paths) < FUNC_CP_MAX)
			create_func_code_paths(pool);
		atomic_inc(paths);
		pop_func_codepath();
		return;
	}

	for (size_t i = 0; i < cp->branches; i++) {
		if (!cp->next[i])
			continue;

		int checked = check_func_code_path(cp->next[i]);
		if (checked && flag)
			continue;
		else if (checked)
			gen_func_codepaths(cp->next[i], pool, idx+1, 1, paths);
		else
			gen_func_codepaths(cp->next[i], pool, idx+1, 0, paths);
		if (atomic_read(paths) >= FUNC_CP_MAX)
			break;
	}

	pop_func_codepath();
	return;
}

static void drop_func_codepath(struct path_list *head)
{
	struct codep_list *tmp1, *next1;
	list_for_each_entry_safe(tmp1, next1, &head->path_head, sibling) {
		list_del(&tmp1->sibling);
		free(tmp1);
	}
	free(head);
}

/*
 * ************************************************************************
 * generate code paths in functions,
 * if line0 is 0, from the beginning of fsn_from
 * if line1 is 0, till the beginning of fsn_to
 * if line1 is -1, till the end of fsn_to
 * ************************************************************************
 */
void gen_code_paths(void *arg, struct clib_rw_pool *pool)
{
	struct sinode *fsn_from, *fsn_to;
	long line0, line1;
	long *args = (long *)arg;
	fsn_from = (struct sinode *)args[0];
	line0 = args[1];
	fsn_to = (struct sinode *)args[2];
	line1 = args[3];
	atomic_t *paths = (atomic_t *)args[4];

	if ((fsn_from == fsn_to) && (line0 == 0) && (line1 == -1)) {
		struct func_node *fn = (struct func_node *)fsn_from->data;
		if (fn)
			gen_func_codepaths(fn->codes, pool, 0, 0, paths);
		return;
	}

	/* TODO */
}

void drop_code_path(struct path_list *head)
{
	drop_func_codepath(head);
}
