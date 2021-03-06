/* radare - LGPL - Copyright 2009-2012 - pancake, nibble */

#include <r_anal.h>
#include <r_util.h>

R_API struct r_anal_refline_t *r_anal_reflines_get(struct r_anal_t *anal,
	ut64 addr, ut8 *buf, ut64 len, int nlines, int linesout, int linescall)
{
	RAnalRefline *list2, *list = R_NEW (RAnalRefline);
	RAnalOp op = {0};
	ut8 *ptr = buf;
	ut8 *end = buf + len;
	ut64 opc = addr;
	int sz = 0, index = 0;

	INIT_LIST_HEAD (&(list->list));

	end -= 8; // XXX Fix some segfaults when r_anal backends are buggy
	/* analyze code block */
	while (ptr<end) {
		if (nlines != -1 && --nlines == 0)
			break;
#if 0
		if (config.interrupted)
			break;
		int dt = data_type(config.seek+bsz);
		if (dt != DATA_FUN && dt != DATA_CODE) {
			ut64 sz = data_size (config.seek+bsz);
			if (sz > 0) {
				ptr += sz;
				bsz += sz;
				continue;
			}
		}
#endif

		addr += sz;
		// This can segflauta if opcode length and buffer check fails
		r_anal_op_fini (&op);
		sz = r_anal_op (anal, &op, addr, ptr, (int)(end-ptr));
		if (sz > 0) {
			/* store data */
			switch(op.type) {
			case R_ANAL_OP_TYPE_CALL:
				if (!linescall)
					break;
			case R_ANAL_OP_TYPE_CJMP:
			case R_ANAL_OP_TYPE_JMP:
				if (!linesout && (op.jump > opc+len || op.jump < opc))
					goto __next;
				if (op.jump == 0LL)
					goto __next;
				list2 = R_NEW (RAnalRefline);
				list2->from = addr;
				list2->to = op.jump;
				list2->index = index++;
				list_add_tail (&(list2->list), &(list->list));
				break;
			}
		} else sz = 1;
	__next:
		ptr += sz;
	}
	r_anal_op_fini (&op);
	return list;
}

// TODO: this is TOO SLOW. do not iterate over all reflines or gtfo
/* umf..this should probably be outside this file */
R_API char* r_anal_reflines_str(RAnal *anal, RAnalRefline *list, ut64 addr, int opts) {
	int linestyle = opts & R_ANAL_REFLINE_TYPE_STYLE;
	int wide = opts & R_ANAL_REFLINE_TYPE_WIDE;
	char ch = ' ', *str = NULL;
	struct list_head *pos;
	RAnalRefline *ref;
	int dir = 0;

	if (!list)
		return NULL;
	str = r_str_concat (str, " ");
	for (pos = linestyle?(&(list->list))->next:(&(list->list))->prev;
		pos != (&(list->list)); pos = linestyle?pos->next:pos->prev) {
		ref = list_entry (pos, RAnalRefline, list);

		if (addr == ref->to) dir = 1;
		else if (addr == ref->from) dir = 2;
		// TODO: if dir==1
		if (addr == ref->to) {
			str = r_str_concat (str, (ref->from>ref->to)?".":"`");
			ch = '-';
		} else if (addr == ref->from) {
			str = r_str_concat (str, (ref->from>ref->to)?"`":",");
			ch = '=';
		} else if (ref->from < ref->to) { /* down */
			if (addr > ref->from && addr < ref->to) {
				if (ch=='-'||ch=='=')
					str = r_str_concatch (str, ch);
				else str = r_str_concatch (str, '|');
			} else str = r_str_concatch (str, ch);
		} else { /* up */
			if (addr < ref->from && addr > ref->to) {
				if (ch=='-'||ch=='=')
					str = r_str_concatch (str, ch);
				else str = r_str_concatch (str, '|');
			} else str = r_str_concatch (str, ch);
		}
		if (wide)
			str = r_str_concatch (str, (ch=='='||ch=='-')?ch:' ');
	}
	str = r_str_concat (str, (dir==1)?"-> ":(dir==2)?"=< ":"   ");
	if (anal->lineswidth>0) {
		int len = strlen (str);
		if (len>anal->lineswidth)
			r_str_cpy (str, str+len-anal->lineswidth);
	}
{
	int l = anal->lineswidth-strlen (str);
	while (l-->0) {
		str = r_str_prefix (str, " ");
	}
}
	return str;
}

R_API int r_anal_reflines_middle(RAnal *anal, RAnalRefline *list, ut64 addr, int len) {
	struct list_head *pos;
	for (pos = (&(list->list))->next; pos != (&(list->list)); pos = pos->next) {
		RAnalRefline *ref = list_entry (pos, RAnalRefline, list);
		if ((ref->to> addr) && (ref->to < addr+len))
			return R_TRUE;
	}
	return R_FALSE;
}
