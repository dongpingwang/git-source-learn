/*
 * Various "current state", notably line numbers and what
 * file (and how) we're patching right now.. The "is_xxxx"
 * things are flags, where -1 means "don't know yet".
 */
static int linenr = 1;
static int old_mode, new_mode;
static char *old_name, *new_name, *def_name;
static int is_rename, is_copy, is_new, is_delete;

#define SLOP (16)

	/*
	 * Make sure that we have some slop in the buffer
	 * so that we can do speculative "memcmp" etc, and
	 * see to it that it is NUL-filled.
	 */
	if (alloc < size + SLOP)
		buffer = xrealloc(buffer, size + SLOP);
	memset(buffer + size, 0, SLOP);
static int is_dev_null(const char *str)
{
	return !memcmp("/dev/null", str, 9) && isspace(str[9]);
}

#define TERM_EXIST	1
#define TERM_SPACE	2
#define TERM_TAB	4

static int name_terminate(const char *name, int namelen, int c, int terminate)
{
	if (c == ' ' && !(terminate & TERM_SPACE))
		return 0;
	if (c == '\t' && !(terminate & TERM_TAB))
		return 0;

	/*
	 * Do we want an existing name? Return false and
	 * continue if it's not there.
	 */
	if (terminate & TERM_EXIST)
		return cache_name_pos(name, namelen) >= 0;

	return 1;
}

static char * find_name(const char *line, char *def, int p_value, int terminate)
{
	int len;
	const char *start = line;
	char *name;

	for (;;) {
		char c = *line;

		if (isspace(c)) {
			if (c == '\n')
				break;
			if (name_terminate(start, line-start, c, terminate))
				break;
		}
		line++;
		if (c == '/' && !--p_value)
			start = line;
	}
	if (!start)
		return def;
	len = line - start;
	if (!len)
		return def;

	/*
	 * Generally we prefer the shorter name, especially
	 * if the other one is just a variation of that with
	 * something else tacked on to the end (ie "file.orig"
	 * or "file~").
	 */
	if (def) {
		int deflen = strlen(def);
		if (deflen < len && !strncmp(start, def, deflen))
			return def;
	}

	name = xmalloc(len + 1);
	memcpy(name, start, len);
	name[len] = 0;
	free(def);
	return name;
}

/*
 * Get the name etc info from the --/+++ lines of a traditional patch header
 *
 * NOTE! This hardcodes "-p1" behaviour in filename detection.
 *
 * FIXME! The end-of-filename heuristics are kind of screwy. For existing
 * files, we can happily check the index for a match, but for creating a
 * new file we should try to match whatever "patch" does. I have no idea.
 */
static void parse_traditional_patch(const char *first, const char *second)
{
	int p_value = 1;
	char *name;

	first += 4;	// skip "--- "
	second += 4;	// skip "+++ "
	if (is_dev_null(first)) {
		is_new = 1;
		name = find_name(second, def_name, p_value, TERM_SPACE | TERM_TAB);
		new_name = name;
	} else if (is_dev_null(second)) {
		is_delete = 1;
		name = find_name(first, def_name, p_value, TERM_EXIST | TERM_SPACE | TERM_TAB);
		old_name = name;
	} else {
		name = find_name(first, def_name, p_value, TERM_EXIST | TERM_SPACE | TERM_TAB);
		name = find_name(second, name, p_value, TERM_EXIST | TERM_SPACE | TERM_TAB);
		old_name = new_name = name;
	}
	if (!name)
		die("unable to find filename in patch at line %d", linenr);
}

static int gitdiff_hdrend(const char *line)
{
	return -1;
}

/*
 * We're anal about diff header consistency, to make
 * sure that we don't end up having strange ambiguous
 * patches floating around.
 *
 * As a result, gitdiff_{old|new}name() will check
 * their names against any previous information, just
 * to make sure..
 */
static char *gitdiff_verify_name(const char *line, int isnull, char *orig_name, const char *oldnew)
	int len;
	const char *name;

	if (!orig_name && !isnull)
		return find_name(line, NULL, 1, 0);

	name = "/dev/null";
	len = 9;
	if (orig_name) {
		name = orig_name;
		len = strlen(name);
		if (isnull)
			die("git-apply: bad git-diff - expected /dev/null, got %s on line %d", name, linenr);
	}

	if (*name == '/')
		goto absolute_path;

		char c = *line++;
		if (c == '\n')
		if (c != '/')
			continue;
absolute_path:
		if (memcmp(line, name, len) || line[len] != '\n')
			break;
		return orig_name;
	die("git-apply: bad git-diff - inconsistent %s filename on line %d", oldnew, linenr);
	return NULL;
}

static int gitdiff_oldname(const char *line)
{
	old_name = gitdiff_verify_name(line, is_new, old_name, "old");
	return 0;
}

static int gitdiff_newname(const char *line)
{
	new_name = gitdiff_verify_name(line, is_delete, new_name, "new");
	return 0;
}

static int gitdiff_oldmode(const char *line)
{
	old_mode = strtoul(line, NULL, 8);
	return 0;
}

static int gitdiff_newmode(const char *line)
{
	new_mode = strtoul(line, NULL, 8);
	return 0;
}

static int gitdiff_delete(const char *line)
{
	is_delete = 1;
	return gitdiff_oldmode(line);
}

static int gitdiff_newfile(const char *line)
{
	is_new = 1;
	return gitdiff_newmode(line);
}

static int gitdiff_copysrc(const char *line)
{
	is_copy = 1;
	old_name = find_name(line, NULL, 0, 0);
	return 0;
}

static int gitdiff_copydst(const char *line)
{
	is_copy = 1;
	new_name = find_name(line, NULL, 0, 0);
	return 0;
}

static int gitdiff_renamesrc(const char *line)
{
	is_rename = 1;
	old_name = find_name(line, NULL, 0, 0);
	return 0;
}

static int gitdiff_renamedst(const char *line)
{
	is_rename = 1;
	new_name = find_name(line, NULL, 0, 0);
	return 0;
}

static int gitdiff_similarity(const char *line)
{
	return 0;
}

/*
 * This is normal for a diff that doesn't change anything: we'll fall through
 * into the next diff. Tell the parser to break out.
 */
static int gitdiff_unrecognized(const char *line)
{
	return -1;
static int parse_git_header(char *line, int len, unsigned int size)
	unsigned long offset;

	/* A git diff has explicit new/delete information, so we don't guess */
	is_new = 0;
	is_delete = 0;

	line += len;
	size -= len;
	linenr++;
	for (offset = len ; size > 0 ; offset += len, size -= len, line += len, linenr++) {
		static const struct opentry {
			const char *str;
			int (*fn)(const char *);
		} optable[] = {
			{ "@@ -", gitdiff_hdrend },
			{ "--- ", gitdiff_oldname },
			{ "+++ ", gitdiff_newname },
			{ "old mode ", gitdiff_oldmode },
			{ "new mode ", gitdiff_newmode },
			{ "deleted file mode ", gitdiff_delete },
			{ "new file mode ", gitdiff_newfile },
			{ "copy from ", gitdiff_copysrc },
			{ "copy to ", gitdiff_copydst },
			{ "rename from ", gitdiff_renamesrc },
			{ "rename to ", gitdiff_renamedst },
			{ "similarity index ", gitdiff_similarity },
			{ "", gitdiff_unrecognized },
		};
		int i;
		if (!len || line[len-1] != '\n')
		for (i = 0; i < sizeof(optable) / sizeof(optable[0]); i++) {
			const struct opentry *p = optable + i;
			int oplen = strlen(p->str);
			if (len < oplen || memcmp(p->str, line, oplen))
				continue;
			if (p->fn(line + oplen) < 0)
				return offset;
		}
	return offset;
}

static int parse_num(const char *line, int len, int offset, const char *expect, unsigned long *p)
{
	char *ptr;
	int digits, ex;

	if (offset < 0 || offset >= len)
		return -1;
	line += offset;
	len -= offset;

	if (!isdigit(*line))
		return -1;
	*p = strtoul(line, &ptr, 10);

	digits = ptr - line;

	offset += digits;
	line += digits;
	len -= digits;

	ex = strlen(expect);
	if (ex > len)
		return -1;
	if (memcmp(line, expect, ex))
		return -1;

	return offset + ex;
}

/*
 * Parse a unified diff fragment header of the
 * form "@@ -a,b +c,d @@"
 */
static int parse_fragment_header(char *line, int len, unsigned long *pos)
{
	int offset;

	if (!len || line[len-1] != '\n')
		return -1;

	/* Figure out the number of lines in a fragment */
	offset = parse_num(line, len, 4, ",", pos);
	offset = parse_num(line, len, offset, " +", pos+1);
	offset = parse_num(line, len, offset, ",", pos+2);
	offset = parse_num(line, len, offset, " @@", pos+3);

	return offset;
	is_rename = is_copy = 0;
	is_new = is_delete = -1;
	old_mode = new_mode = 0;
	def_name = old_name = new_name = NULL;
	for (offset = 0; size > 0; offset += len, size -= len, line += len, linenr++) {

		/*
		 * Make sure we don't find any unconnected patch fragmants.
		 * That's a sign that we didn't find a header, and that a
		 * patch has become corrupted/broken up.
		 */
		if (!memcmp("@@ -", line, 4)) {
			unsigned long pos[4];
			if (parse_fragment_header(line, len, pos) < 0)
				continue;
			error("patch fragment without header at line %d: %.*s", linenr, len-1, line);
		}

			int git_hdr_len = parse_git_header(line, len, size);
			*hdrsize = git_hdr_len;
		parse_traditional_patch(line, line+len);
		linenr += 2;
	unsigned long pos[4], oldlines, newlines;
	offset = parse_fragment_header(line, len, pos);
	oldlines = pos[1];
	newlines = pos[3];

	if (is_new < 0 && (pos[0] || oldlines))
		is_new = 0;
	if (is_delete < 0 && (pos[1] || newlines))
		is_delete = 0;
	linenr++;
	for (offset = len; size > 0; offset += len, size -= len, line += len, linenr++) {
			die("corrupt patch at line %d", linenr);
printf("Rename: %d\n", is_rename);
printf("Copy:   %d\n", is_copy);
printf("New:    %d\n", is_new);
printf("Delete: %d\n", is_delete);
printf("Mode:   %o:%o\n", old_mode, new_mode);
printf("Name:   '%s':'%s'\n", old_name, new_name);

	if (old_name && cache_name_pos(old_name, strlen(old_name)) < 0)
		die("file %s does not exist", old_name);
	if (new_name && (is_new | is_rename | is_copy)) {
		if (cache_name_pos(new_name, strlen(new_name)) >= 0)
			die("file %s already exists", new_name);
	}
	int read_stdin = 1;
			read_stdin = 0;
		read_stdin = 0;
	if (read_stdin)
		apply_patch(0);