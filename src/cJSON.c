#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

// 常量指针, 指针的指向可以改变, 指针指向的内容不能修改
static const char *ep;

// 获取错误信息
const char *cJSON_GetErrorPtr(void) { return ep; }
// 不区分大小写比较, s2是否包含s1
static int cJSON_strcasecmp(const char *s1, const char *s2) 
{
	if (s1 == NULL) return (s1 == s2) ? 0 : 1;
	if (s2 == NULL) return 1;
	for (; tolower(*s1) == tolower(*s2); ++s1, ++s2) {
		if (*s1 == '\0') {
			return 0;
		}
	}
	return tolower(*(const unsigned char*)s1) - tolower(*(const unsigned char*)s2);
}
// 函数指针, cJSON_malloc指向malloc, 返回值为void*
static void *(*cJSON_malloc)(size_t sz) = malloc;
// 函数指针, cJSON_free指向malloc, 返回值为void
static void (*cJSON_free)(void *ptr) = free;
// 拷贝str, 返回副本
static char *cJSON_strdup(const char* str)
{
	size_t len;
	char *copy;
	len = strlen(str) + 1;
	copy = (char*)cJSON_malloc(len);
	if (copy == NULL) {
		return 0;
	}
	memcpy(copy, str, len);
	return copy;
}
// 初始化cJSON_malloc和cJSON_free
void cJSON_InitHooks(cJSON_Hooks* hooks)
{
	if (!hooks) {
		cJSON_malloc = malloc;
		cJSON_free = free;
		return;
	}

	cJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
	cJSON_free 	 = (hooks->free_fn) ? hooks->free_fn : free;
}
// 创建一个新的cJSON对象
static cJSON *cJSON_New_Item(void)
{
	cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
	if (node) memset(node, 0, sizeof(cJSON));
	return node;
}
// 递归删除cJSON的所有项
void cJSON_Delete(cJSON *now) 
{
	cJSON *next;
	while (now) {
		next = now->next;
		if (!(now->type & cJSON_IsReference) && now->child) cJSON_Delete(now->child);
		if (!(now->type & cJSON_IsReference) && now->valuestring) cJSON_free(now->valuestring);
		if (!(now->type & cJSON_StringIsConst) && now->string) cJSON_free(now->string);
		cJSON_free(now);
		now = next;
	}
}

// 解析数字, 将字符串解析为数字存储在cJSON中, 返回num
static const char *parse_number(cJSON* item, const char* num) 
{
	double n = 0; // 整数和小数的大小
	double sign = 1; // 数字的符号
	double scale = 0; // 小数的位数
	int subscale = 0; // 指数
	int signsubscale = 1; // 指数的符号

	if (*num == '-') sign = -1, num++;
	if (*num == '0') num++; 	// 可能存在多个前导零, 可以考虑改为while
	// 整数部分
	if (*num >= '1' && *num <= '9') {
		do {
			n = n * (10.0) + (*num++ - '0');
		}while (*num >= '0' && *num <= '9');
	}
	// 小数部分
	if (*num == '.' && num[1] >= '0' && num[1] <= '9') {
		num++;
		do {
			n = n * (10.0) + (*num++ - '0');
			scale--;
		}while (*num >= '0' && *num <= '9');
	}

	// 指数部分
	if (*num == 'e' || *num == 'E') {
		num++;
		if (*num == '+') {
			num++;
		}else if (*num == '-') {
			signsubscale = -1;
			num++;
		}
		while (*num >= '0' && *num <= '9') {
			subscale = (subscale * 10) + (*num++ - '0');
		}
	}
	// 计算小数点和指数之后的大小
	n = sign * n * pow(10.0, (scale + subscale * signsubscale));
	item->valuedouble = n;
	item->valueint = (int)n;
	item->type = cJSON_Number;
	return num;
}

// int范围内, 返回大于等于x的最小2的幂次数
static int pow2gt(int x) {
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

// 返回满足needed长度的字符串地址
static char *ensure(printbuffer *p, int needed)
{
	char *newbuffer; // 返回的地址
	int newsize;
	if (!p || !p->buffer) {
		return 0;
	}
	needed += p->offset; // 加上needed所需要的总长度
	if(needed <= p->length) { // 如果buffer满足, 返回地址
		return p->buffer + p->offset;
	}
	newsize = pow2gt(needed); // 重新定义地址
	newbuffer = (char*) cJSON_malloc(newsize); // 申请空间
	if(!newbuffer) {
		cJSON_free(p->buffer);
		p->length = 0;
		p->buffer = 0;
		return 0;
	}else {
		memcpy(newbuffer, p->buffer, p->length); // 把buffer之前的内容拷贝到新地址
	}
	// 释放内存, 修改buffer
	cJSON_free(p->buffer);
	p->length = newsize;
	p->buffer = newbuffer;
	return newbuffer + p->offset;
}

// 更新buffer的长度, 返回已经使用的长度
static int update(printbuffer *p)
{
	char *str;
	if (!p || !p->buffer) {
		return 0;
	}
	str = p->buffer + p->offset;
	return p->offset + strlen(str);
}
// 从cJSON中把数字转为字符串, 返回字符串地址
static char *print_number(cJSON *item, printbuffer *p) 
{
	char *str = NULL;
	double d = item->valuedouble;
	if (d == 0) {
		if (p != NULL) {
			str = ensure(p, 2);
		}else {
			str = (char*) cJSON_malloc(2);
		}
		if (str != NULL) {
			strcpy(str, "0"); // 同时拷贝'\0'
		}
	}else if (fabs(((double)item->valueint) - d) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) { // 数字可以用int(整数)表示, 忽略小数部分
		if (p != NULL) {
			str = ensure(p, 21);
		}else {
			str = (char*)cJSON_malloc(21);
		}
		if (str != NULL) {
			sprintf(str, "%d", item->valueint);
		}
	}else {
		if (p) str = ensure(p, 64);
		else str = (char*) cJSON_malloc(64);
		if (str) {
			if (fabs(floor(d) - d) <= DBL_EPSILON && fabs(d) < 1.0e60) sprintf(str, "%.0f", d); // 可以忽略小数
			else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9) sprintf(str, "%e", d); // 用科学计数表示
			else sprintf(str, "%f", d);
		}
	}
	return str;
}
// 解析字符串对应的16进制数, 返回整数
static unsigned parse_hex4(const char* str) 
{
	unsigned h = 0;
	for (int i = 0; i < 4; ) {
		if (*str >= '0' && *str <= '9') {
			h += (*str) - '0';
		}else if ((*str >= 'A' && *str <= 'F') || (*str >= 'a' && *str <= 'z')) {
			h += 10 + tolower(*str) - 'a';
		}else {
			return 0;
		}
		i++;
		if (i < 4) { // 十六进制由4个比特位组成
			h <<= 4;
			str++;
		}
	}
	return h;
}

/* 
   firstByteMark为UTF8编码的首字节标记
   大于1字节的有特殊的标记, n字节的编码, 首字节的前n位置1, 第n+1置0, 后面字节的前两位设置为10, 其他的为unicode编码
*/
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
// 将文本解析为未转义的cstring，然后保存在cJSON的valuestring, 解析之后返回下个未解析的地址
static const char *parse_string(cJSON *item, const char *str) 
{
	const char *ptr = str + 1;
	char *ptr2, *out;
	int len = 0;
	unsigned uc, uc2;
	if (*str != '\"') {
		ep = str;
		return 0;
	}
	while(*ptr != '\"' && *ptr && ++len) {
		if (*ptr++ == '\\') ptr++; // 忽略转义符号
	}
	out = (char*) cJSON_malloc(len + 1);
	if (!out) return 0;
	ptr = str + 1;
	ptr2 = out;
	while (*ptr != '\"' && *ptr != '\0') {
		if (*ptr != '\\') {
			*ptr2++ = *ptr++;
		}else {
			ptr++;
			switch(*ptr) {
				case 'b': *ptr2++ = '\b'; break;
				case 'f': *ptr2++ = '\f'; break;
				case 'n': *ptr2++ = '\n'; break;
				case 'r': *ptr2++ = '\r'; break;
				case 't': *ptr2++ = '\t'; break;
				case 'u': /*utf16 to utf8 */ 
					uc = parse_hex4(ptr + 1);
					ptr += 4;
					/*
						2个字节的Unicode和2个字节的UTF16编码相同
						4个字节的Unicode需要4个字节的UTF16编码：
						高比特位使用 D800 - DBFF 存储
						低比特位使用 DC00 - DFFF 存储
						高位和低位的有效位都是10位, 使用 & 3FF获取, 高位左移10位加上低位
						在整个的基础上加上0x10000, 4字节编码的起始范围
					*/
					if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0) break; // 前半部分无效
					if (uc >= 0xD800 && uc <= 0xDBFF) { // 高位
						if (ptr[1] != '\\' || ptr[2] != 'u') break; // 后半部分丢失
						uc2 = parse_hex4(ptr+3);
						ptr += 6;
						if (uc2 < 0xDC00 || uc2 > 0xDFFF) break; // 后半部分无效
						uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
					}
					len = 4;
					// 判断编码的字节数
					if(uc < 0x80) len = 1;
					else if (uc < 0x800) len = 2;
					else if (uc < 0x10000) len = 3;
					ptr2 += len;
					switch (len) {
						/*
							大端模式, 数据的高字节保存在内存低地址
							(uc | 0x80): 多字节的编码, 除了首字节, 其他为10开始
							(& 0xBF)   : 得到剩余6位
							(uc >>= 6) : 处理下个数据
						*/
						case 4: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 = (uc | firstByteMark[len]); // 首字节标志
					}
					ptr2 += len;
					break;
				default: *ptr2++ = *ptr; break;
			}
			ptr++;
		}
	}
	*ptr2 = 0;
	if (*ptr == '\"') ptr++;
	item->valuestring = out;
	item->type = cJSON_String;
	return ptr;
}
// 将提供的cstring转为可以打印的转义版本, 返回字符串地址
static char *print_string_ptr(const char *str, printbuffer *p)
{
	const char *ptr;
	char *ptr2, *out;
	int len = 0;
	int flag = 0;
	unsigned char token;

	// 判断是否有控制字符, 引号, 转义字符
	for (ptr = str; *ptr; ptr++) {
		flag |= ((*ptr > 0 && *ptr < 32) || (*ptr == '\"') || (*ptr == '\\')) ? 1 : 0;
	}	
	if (!flag) { // 没有转义字符
		len = ptr - str; // 字符串的长度
		// buffer申请长度为len+3, 包含""和'\0'
		if (p) out = ensure(p, len+3);
		else out = (char*)cJSON_malloc(len+3);
		if (!out) return 0;
		ptr2 = out;
		*ptr2++ = '\"';
		strcpy(ptr2, str);
		ptr2[len] = '\"';
		ptr2[len+1] = '\0';
		return out;
	}
	// 如果str为空, 返回""
	if (!str) {
		if (p) out = ensure(p, 3);
		else out = (char*)cJSON_malloc(3);
		if (!out) return 0;
		strcpy(out, "\"\"");
		return out;
	}
	ptr = str;
	while ((token = *ptr) && ++len) {
		if (strchr("\"\\\b\f\n\r\t", token)) len++; // 转义字符占2字节
		else if(token < 32) len += 5; // 控制字符占6个字节
		ptr++;
	}
	// 申请空间
	if (p) out = ensure(p, len+3);
	else out = (char*)cJSON_malloc(len+3);
	if (!out) return 0;
	ptr2 = out;
	ptr = str;
	*ptr2++ = '\"';
	while (*ptr) {
		// 非转义字符
		if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') *ptr2++ = *ptr++;
		else {
			// 转义字符
			*ptr2++ = '\\';
			switch (token = *ptr++) {
				case '\\': 	*ptr2++ = '\\';	break;
				case '\"': 	*ptr2++ = '\"';	break;
				case '\b': 	*ptr2++ = 'b';	break;
				case '\f': 	*ptr2++ = 'f';	break;
				case '\n': 	*ptr2++ = 'n'; 	break;
				case '\t': 	*ptr2++ = 't'; 	break;
				default: 	sprintf(ptr2, "u%04x", token);
							ptr2 += 5;
							break;
			}
		}
	}
	*ptr2++ = '\"';
	*ptr2++ = 0;
	return out;
}
// 返回待打印的字符串
static char* print_string(cJSON* item, printbuffer *p) 
{
	return print_string_ptr(item->valuestring, p);
}
// 跳过控制字符
static const char *skip(const char *in)
{
	while (in && *in && (unsigned char) *in <= 32) in++;
	return in;
}

static const char* parse_value(cJSON* item, const char *value);
static char* print_value(cJSON* item, int depth, int fmt, printbuffer *p);
static const char* parse_array(cJSON* item, const char* value);
static char* print_array(cJSON* item, int depth, int fmt, printbuffer *p);
static const char* parse_object(cJSON* item, const char *value);
static char* print_object(cJSON* item, int depth, int fmt, printbuffer* p);

// 解析对象, 调用parse_value完成解析
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated)
{
	const char *end = 0;
	cJSON *c = cJSON_New_Item();
	ep = 0;
	if (!c) return 0;
	end = parse_value(c, skip(value));
	if (!end) {
		cJSON_Delete(c);
		return 0;
	}
	// 以'\0'做结尾
	if (require_null_terminated) {
		end = skip(end);
		if (*end) {
			cJSON_Delete(c);
			ep = end;
			return 0;
		}
	}
	// 返回解析剩余的地址
	if (return_parse_end) *return_parse_end = end;
	return c;
}
// 解析代码, 通过调用cJSON_ParseWithOpts完成
cJSON *cJSON_Parse(const char *value) 
{
	return cJSON_ParseWithOpts(value, 0, 0);
}
// 获取(格式化)cJSON打印文本, 通过调用print_value完成
char *cJSON_Print(cJSON *item) 
{
	return print_value(item, 0, 1, 0);
}
// 获取(非格式化)cJSON打印文本, 通过调用print_value完成
char *cJSON_PrintUnformatted(cJSON *item) {
	return print_value(item, 0, 0, 0);
}
// 
char *cJSON_Printbuffered(cJSON *item, int prebuffer, int fmt) {
	printbuffer p;
	p.buffer = (char*) cJSON_malloc(prebuffer);
	p.length = prebuffer;
	p.offset = 0;
	return print_value(item, 0, fmt, &p);
	return p.buffer;
}
// 解析器核心-遇到文本时，进行适当处理
static const char *parse_value(cJSON *item, const char *value) 
{
	if (!value)	return 0;
	if (!strncmp(value, "null", 4)) {
		item->type = cJSON_NULL;
		return value + 4;
	}
	if (!strncmp(value, "false", 5)) {
		item->type = cJSON_False;
		return value + 5;
	}
	if (!strncmp(value, "true", 4)) {
		item->type = cJSON_True;
		item->valueint = 1;
		return value + 4;
	}
	if (*value == '\"') return parse_string(item, value);
	if (*value == '-' || (*value >= '0' && *value <= '9')) return parse_number(item, value);
	if (*value == '[') return parse_array(item, value);
	if (*value == '{') return parse_object(item, value);
	ep = value;
	return 0;
}
// 根据cJSON的type调用对应的print函数
static char *print_value(cJSON *item, int depth, int fmt, printbuffer *p) {
	char *out = 0;
	if (!item) return 0;
	if (p) {
		switch((item->type) & 255) {
			case cJSON_NULL: 
				out = ensure(p, 5);
				if (out) strcpy(out, "null");
				break;
			case cJSON_False:
				out = ensure(p, 6);
				if (out) strcpy(out, "false");
				break;
			case cJSON_True:
				out = ensure(p, 5);
				if (out) strcpy(out, "true");
				break;
			case cJSON_Number:
				out = print_number(item, p);
				break;
			case cJSON_String:
				out = print_string(item, p);
				break;
			case cJSON_Array:
				out = print_array(item, depth, fmt, p);
				break;
			case cJSON_Object:
				out = print_object(item, depth, fmt, p);
				break;
		}
	} else {
		switch ((item->type) & 255) {
			case cJSON_NULL: 	out = cJSON_strdup("null");		break;
			case cJSON_False:	out = cJSON_strdup("false"); 	break;
			case cJSON_True:	out = cJSON_strdup("true");		break;
			case cJSON_Number:	out = print_number(item, 0);	break;
			case cJSON_String:	out = print_string(item, 0);	break;
			case cJSON_Array:	out = print_array(item, depth, fmt, 0);		break;
			case cJSON_Object:	out = print_object(item, depth, fmt, 0);	break;
		}
	}
	return out;
}
// 将文本解析为数组, 返回解析后的地址
static const char *parse_array(cJSON *item, const char *value) {
	cJSON *child;
	if (*value != '[') {
		ep = value;
		return 0;
	}
	item->type = cJSON_Array;
	value = skip(value + 1);
	// 判断是否为空 []
	if (*value == ']') return value + 1;

	item->child = child = cJSON_New_Item();
	if (!item->child) return 0;
	// 解析数组中的成员
	value = skip(parse_value(child, skip(value)));
	if (!value) return 0;
	// 同级用双向链表连接
	while (*value == ',') {
		cJSON *new_item = cJSON_New_Item();
		if (!new_item) return 0;
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		// 解析数组中的成员
		value = skip(parse_value(child, skip(value+1)));
		if (!value) return 0;
	}

	if (*value == ']') return value + 1;
	ep = value;
	return 0;
}
// 将cJSON中的数组转为文本, 返回打印字符串的地址
static char *print_array(cJSON *item, int depth, int fmt, printbuffer *p) {
	char **entries;
	char *out = 0, *ptr, *ret;
	int len = 5;
	cJSON *child = item->child;
	int numentries = 0, i = 0, fail = 0;
	size_t tmplen = 0;

	while (child) {
		numentries++;
		child = child->next;
	}
	// 数组为空
	if (!numentries) {
		if (p) 	out = ensure(p, 3);
		else 	out = (char*) cJSON_malloc(3);
		if (out) strcpy(out, "[]");
		return out;
	}

	if (p) { // 有buffer
		i = p->offset;
		ptr = ensure(p, 1);
		if (!ptr) return 0;
		*ptr = '[';
		p->offset++;
		child = item->child;
		while (child && !fail) {
			print_value(child, depth+1, fmt, p);
			p->offset = update(p);
			if (child->next) {
				len = fmt ? 2 : 1;
				ptr = ensure(p, len+1);
				if (!ptr) return 0;
				*ptr++ = ',';
				if (fmt) *ptr++ = ' ';
				*ptr = 0;
				p->offset += len;
			}
			child = child->next;
		}
		ptr = ensure(p, 2);
		if (!ptr) return 0;
		*ptr++ = ']';
		*ptr = 0;
		out = (p->buffer) + i;
	}else { // 没有buffer
		// 申请空间并初始化
		entries = (char**) cJSON_malloc(numentries * sizeof(char*));
		if (!entries) return 0;
		memset(entries, 0, numentries * sizeof(char*));
		child = item->child;
		while (child && !fail) {
			// 数组成员可能包含对象, 使用print_value处理
			ret = print_value(child, depth+1, fmt, 0);
			entries[i++] = ret;
			if (ret) len += strlen(ret) + 2 + (fmt ? 1 : 0);
			else fail = 1;
			child = child->next;
		}
		// 申请空间
		if (!fail) out = (char*) cJSON_malloc(len);
		if (!out) fail = 1;
		if (fail) {
			for (i = 0; i < numentries; ++i) {
				if (entries[i]) cJSON_free(entries[i]);
			}
			cJSON_free(entries);
			return 0;
		}
		*out = '[';
		ptr = out + 1;
		*ptr = 0;
		for (i = 0; i < numentries; ++i) {
			tmplen = strlen(entries[i]);
			memcpy(ptr, entries[i], tmplen);
			ptr += tmplen;
			if (i != numentries - 1) {
				*ptr++ = ',';
				if (fmt) {
					*ptr++ = ' ';
					*ptr = 0;
				}
			}
			cJSON_free(entries[i]);
		}
		cJSON_free(entries);
		*ptr++ = ']';
		*ptr++ = 0;
	}
	return out;
}

// 从文本中解析对象, 返回解析后的地址
static const char *parse_object(cJSON *item, const char *value) {
	cJSON *child;
	if (*value != '{') {
		ep = value;
		return 0;
	}
	// 判断是否为空 {}
	item->type = cJSON_Object;
	value = skip(value + 1);
	if (*value == '}') return value + 1;
	// 解析key, 存到cJSON->string中
	item->child = child = cJSON_New_Item();
	if (!child) return 0;
	value = skip(parse_string(child, skip(value)));
	if (!value) return 0;
	child->string = child->valuestring;
	child->valuestring = 0;
	if (*value != ':') {
		ep = value;
		return 0;
	}
	// 解析value
	value = skip(parse_value(child, skip(value + 1)));
	if (!value) return 0;
	// 同级的用双向链表连接
	while (*value == ',') {
		cJSON *new_item;
		new_item = cJSON_New_Item();
		if (!new_item) return 0;
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		// 解析key
		value = skip(parse_string(child, skip(value+1)));
		if (!value) return 0;
		child->string = child->valuestring;
		child->valuestring = 0;
		if (*value != ':') {
			ep = value;
			return 0;
		}
		// 解析value
		value = skip(parse_value(child, skip(value + 1)));
		if (!value) return 0;
 	}

 	if (*value == '}') return value + 1;
 	ep = value;
 	return 0;
}
// 把对象转为文本, 返回打印字符串的地址
static char *print_object(cJSON *item, int depth, int fmt, printbuffer *p) {
	char **entries = 0, **names = 0;
	char *out = 0, *ptr, *ret, *str;
	int len = 7, i = 0, j;
	cJSON *child = item->child;
	int numentries = 0, fail = 0;
	size_t tmplen = 0;
	while (child) {
		numentries++;
		child = child->next;
	}
	// 空对象
	if (!numentries) {
		if (p) out = ensure(p, fmt ? depth+4 : 3);
		else   out = (char*) cJSON_malloc(fmt ? depth+4 : 3);
		if (!out) return 0;
		ptr = out;
		*ptr++ = '{';
		if (fmt) {
			*ptr++ = '\n';
			for (i = 0; i < depth - 1; ++i) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr++ = 0;
		return out;
	}
	// printf("depth = %d\n", depth);
	if (p) { // 有buffer
		i = p->offset;
		len = fmt ? 2 : 1;
		ptr = ensure(p, len+1);
		if (!ptr) return 0;
		*ptr++ = '{';
		if (fmt) *ptr++ = '\n';
		*ptr = 0;
		p->offset += len;
		child = item->child;
		depth++;
		while (child) {
			if (fmt) {
				ptr = ensure(p, depth);
				if (!ptr) return 0;
				for (j = 0; j < depth; ++j) {
					*ptr++ = '\t';
				}
				p->offset += depth;
			}
			print_string_ptr(child->string, p);
			p->offset = update(p);

			len = fmt ? 2 : 1;
			ptr = ensure(p, len);
			if (!ptr) return 0;
			*ptr++ = ':';
			if (fmt) *ptr++ = '\t';
			p->offset += len;

			print_value(child, depth, fmt, p);
			p->offset = update(p);

			len = (fmt ? 1 : 0) + child->next ? 1 : 0;
			ptr = ensure(p, len+1);
			if (!ptr) return 0;
			if (child->next) *ptr++ = ',';
			if (fmt) *ptr++ = '\n';
			*ptr = 0;
			p->offset += len;
			child = child->next;
		}
		ptr = ensure(p, fmt ? (depth+1) : 2);
		if (!ptr) return 0;
		if (fmt) {
			for (i = 0; i < depth-1; ++i) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr = 0;
		out = (p->buffer) + i;

	} else{ // 没有buffer
		/* 	用两个指针数组保存对象中所有的成员
		   	names保存key
		   	entries保存value
		*/
		// 申请空间并初始化
		entries = (char**) cJSON_malloc(numentries * sizeof(char*));
		if (!entries) return 0;
		names = (char**) cJSON_malloc(numentries * sizeof(char*));
		if (!names) {
			cJSON_free(entries);
			return 0;
		}
		memset(entries, 0, sizeof(char*) * numentries);
		memset(names, 0, sizeof(char*) * numentries);
		// 解析key和value
		child = item->child;
		depth++;
		if (fmt) len += depth;
		while (child && !fail) {
			names[i] = str = print_string_ptr(child->string, 0);
			entries[i++] = ret = print_value(child, depth, fmt, 0);
			if (str && ret) {
				len += strlen(ret) + strlen(str) + 2 + (fmt ? 2+depth : 0);
			}else {
				fail = 1;
			}
			child = child->next;
		}

		if (!fail) out = (char*) cJSON_malloc(len);
		if (!out) fail = 1;
		// 出错处理, 释放空间
		if (fail) {
			for (i = 0; i < numentries; ++i) {
				if (names[i]) cJSON_free(names[i]);
				if (entries[i]) cJSON_free(entries[i]);
			}
			cJSON_free(names);
			cJSON_free(entries);
			return 0;
		}
		// 转为文本, 保存到out
		*out = '{';
		ptr = out + 1;
		if (fmt) *ptr++ = '\n';
		*ptr = 0;
		for (i = 0; i < numentries; ++i) {
			if (fmt) {
				for (j = 0; j < depth; ++j) 
					*ptr++ = '\t';
			}
			tmplen = strlen(names[i]);
			memcpy(ptr, names[i], tmplen);
			ptr += tmplen;
			*ptr++ = ':';
			if (fmt) *ptr++ = '\t';
			strcpy(ptr, entries[i]);
			ptr += strlen(entries[i]);
			if (i != numentries - 1) *ptr++ = ',';
			if (fmt) *ptr++ = '\n';
			*ptr = 0;
			// 释放内存
			cJSON_free(names[i]);
			cJSON_free(entries[i]);
		}
		// 释放内存
		cJSON_free(names);
		cJSON_free(entries);
		if (fmt) {
			for (i = 0; i < depth-1; ++i) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr++ = 0;
	}
	return out;
}
// 获取数组的长度
int cJSON_GetArraySize(cJSON *array) {
	cJSON *c = array->child;
	int i = 0;
	while (c) {
		i++;
		c = c->next;
	}
	return i;
}
// 获取数组中指定的成员
cJSON *cJSON_GetArrayItem(cJSON *array, int item) {
	cJSON *c = array->child;
	while (c && item > 0) {
		item--;
		c = c->next;
	}
	return c;
}
// 获取对象中指定的成员
cJSON *cJSON_GetObjectItem(cJSON *object, const char *string) {
	cJSON *c = object->child;
	while (c && cJSON_strcasecmp(c->string, string)) {
		c = c->next;
	}
	return c;
}
// 连接双向链表中的prev和item
static void suffix_object(cJSON *prev, cJSON *item) {
	prev->next = item;
	item->prev = prev;
}

static cJSON *create_reference(cJSON *item) {
	cJSON *ref = cJSON_New_Item();
	if (!ref) return 0;
	memcpy(ref, item, sizeof(cJSON));
	ref->string = 0;
	ref->type |= cJSON_IsReference;
	ref->next = ref->prev = 0;
	return ref;
}
// 添加item到数组中
void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
	cJSON *c = array->child;
	if (!item) return;
	if (!c) {
		array->child = item;
	}else {
		while (c && c->next) {
			c = c->next;
		}
		suffix_object(c, item);
	}
}
// 添加item到对象中
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
	if (!item) return;
	if (item->string) {
		cJSON_free(item->string);
	}
	item->string = cJSON_strdup(string);
	cJSON_AddItemToArray(object, item);
}
// 
void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item) {
	if (!item) return;
	if (!(item->type & cJSON_StringIsConst) && item->string) {
		cJSON_free(item->string);
	}
	item->string = (char*)string;
	item->type |= cJSON_StringIsConst;
	cJSON_AddItemToArray(object, item);
}

void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) {
	cJSON_AddItemToArray(array, create_reference(item));
}

void cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item) {
	cJSON_AddItemToObject(object, string, create_reference(item));
}
// 从数组中分离指定的item
cJSON *cJSON_DetachItemFromArray(cJSON *array, int which) {
	cJSON *c = array->child;
	while (c && which > 0) {
		which--;
		c = c->next;
	}
	if (!c) return 0;
	// 修改链表的指向
	if (c->prev) c->prev->next = c->next;
	if (c->next) c->next->prev = c->prev;
	if (c == array->child) array->child = c->next;
	c->next = c->prev = 0;
	return c;
}
// 从数组中删除指定的item
void cJSON_DeleteItemFromArray(cJSON *array, int which) {
	cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}
// 从对象中分离指定的对象
cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string) {
	int i = 0;
	cJSON *c = object->child;
	while (c && cJSON_strcasecmp(c->string, string)) {
		c = c->next;
		i++;
	}
	if (c) return cJSON_DetachItemFromArray(object, i);
	return 0;
}
// 从对象中删除指定的对象
void cJSON_DeleteItemFromObject(cJSON *object, const char *string) {
	cJSON_Delete(cJSON_DetachItemFromObject(object, string));
}
// 在特定位置插入item
void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem) {
	cJSON *c = array->child;
	while (c && which > 0) {
		c = c->next;
		which--;
	}
	if (!c) {
		cJSON_AddItemToArray(array, newitem);
		return;
	}
	newitem->next = c;
	newitem->prev = c->prev;
	c->prev = newitem;
	if (c == array->child) {
		array->child = newitem;
	}else {
		c->prev->next = newitem;
	}
}
// 替换数组中指定的item
void cJSON_ReplaceItemInArray (cJSON *array, int which, cJSON *newitem) {
	cJSON *c = array->child;
	while (c && which > 0) {
		c = c->next;
		which--;
	}
	if (!c) return;
	newitem->next = c->next;
	newitem->prev = c->prev;
	if (newitem->next) newitem->next->prev = newitem;
	if (c == array->child) array->child = newitem;
	else newitem->prev->next = newitem;
	c->next = c->prev = 0;
	cJSON_Delete(c);
}
// 替换对象中指定的item
void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem) {
	int i = 0;
	cJSON *c = object->child;
	while (c && cJSON_strcasecmp(c->string, string)) {
		++i;
		c = c->next;
	}
	if (c) {
		newitem->string = cJSON_strdup(string);
		cJSON_ReplaceItemInArray(object, i, newitem);
	}
}
// 创建一个NULL类型的cJSON
cJSON *cJSON_CreateNull(void) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = cJSON_NULL;
	return item;
}
// 创建一个True类型的cJSON
cJSON *cJSON_CreateTrue(void) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = cJSON_True;
	return item;
}
// 创建一个False类型的cJSON
cJSON *cJSON_CreateFalse(void) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = cJSON_False;
	return item;
}
// 创建一个bool类型的cJSON
cJSON *cJSON_CreateBool(int b) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = b ? cJSON_True : cJSON_False;
	return item;
}
// 创建一个Number类型的cJSON
cJSON *cJSON_CreateNumber(double num) {
	cJSON *item = cJSON_New_Item();
	if (item) {
		item->type = cJSON_Number;
		item->valuedouble = num;
		item->valueint = (int)num;
	}
	return item;
}
// 创建一个String类型的cJSON
cJSON *cJSON_CreateString(const char *string) {
	cJSON *item = cJSON_New_Item();
	if (item) {
		item->type = cJSON_String;
		item->valuestring = cJSON_strdup(string);
	}
	return item;
}
// 创建一个Array
cJSON *cJSON_CreateArray(void) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = cJSON_Array;
	return item;
}
// 创建一个Object
cJSON *cJSON_CreateObject(void) {
	cJSON *item = cJSON_New_Item();
	if (item) item->type = cJSON_Object;
	return item;
}
// 创建一个Int类型的数组
cJSON *cJSON_CreateIntArray(const int *numbers, int count) {
	int i;
	cJSON *n = 0, *p = 0, *a = cJSON_CreateArray();
	for (i = 0; a && i < count; ++i) {
		n = cJSON_CreateNumber(numbers[i]);
		if (!i) a->child = n;
		else suffix_object(p, n);
		p = n;
	}
	return a;
}
// 创建一个Float类型的数组
cJSON *cJSON_CreateFloatArray(const float *numbers, int count) {
	int i;
	cJSON *n = 0, *p = 0, *a = cJSON_CreateArray();
	for (i = 0; a && i < count; ++i) {
		n = cJSON_CreateNumber(numbers[i]);
		if (!i) a->child = n;
		else suffix_object(p, n);
		p = n;
	}
	return a;
}
// 创建一个Double类型的数组
cJSON *cJSON_CreateDoubleArray(const double *numbers, int count) {
	int i;
	cJSON *n = 0, *p = 0, *a = cJSON_CreateArray();
	for (i = 0; a && i < count; ++i) {
		n = cJSON_CreateNumber(numbers[i]);
		if (!i) a->child = n;
		else suffix_object(p, n);
		p = n;
	}
	return a;
}
// 创建一个String类型的数组
cJSON *cJSON_CreateStringArray(const char **strings, int count) {
	int i;
	cJSON *n = 0, *p = 0, *a = cJSON_CreateArray();
	for (i = 0; a && i < count; ++i) {
		n = cJSON_CreateString(strings[i]);
		if (!i) a->child = n;
		else suffix_object(p, n);
		p = n;
	}
	return a;
}

cJSON *cJSON_Duplicate(cJSON *item, int recurse) {
	cJSON *newitem, *cptr, *nptr = 0, *newchild;
	if (item) return 0;
	newitem = cJSON_New_Item();
	if (!newitem) return 0;
	newitem->type = item->type & (~cJSON_IsReference);
	newitem->valueint = item->valueint;
	newitem->valuedouble = item->valuedouble;
	if (item->valuestring) {
		newitem->valuestring = cJSON_strdup(item->valuestring);
		if (!newitem->valuestring) {
			cJSON_Delete(newitem);
			return 0;
		}
	}
	if (item->string) {
		newitem->string = cJSON_strdup(item->string);
		if (!newitem->string) {
			cJSON_Delete(newitem);
			return 0;
		}
	}
	// 是否递归复制
	if (!recurse)	return newitem;
	cptr = item->child;
	while (cptr) {
		newchild = cJSON_Duplicate(cptr, 1);
		if (!newchild) {
			cJSON_Delete(newitem);
			return 0;
		}
		if (nptr) {
			nptr->next = newchild;
			newchild->prev = nptr;
			nptr = newchild;
		}else {
			newitem->child = newchild;
			nptr = newchild;
		}
		cptr = cptr->next;
	}
	return newitem;
}
// 简化json
void cJSON_Minify(char *json) {
	char *into = json;
	while (*json) {
		if (*json == ' ' || *json == '\t' ||
		 *json == '\r' || *json == '\n') json++;
		else if (*json == '/' && json[1] == '/') { // 删除单行注释
			while (*json && *json != '\n') json++;
		}else if (*json == '/' && json[1] == '*') { // 删除多行注释
			while (*json && !(*json == '*' && json[1] == '/')) json++;
			json += 2;
		}else if(*json == '\"') {
			*into++ = *json++;
			while (*json && *json != '\"') {
				if (*json == '\\') *into++ = *json++;
				*into++ = *json++;
			}
			*into++ = *json++;
		}else *into++ = *json++;
	}
	*into = 0;
}

