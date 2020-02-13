#ifndef cJSON__H
#define cJSON__H

#ifdef __cpluspluc
extern "C"
{
#endif

/* cJSON Types */
#define cJSON_False 	0
#define cJSON_True 		1
#define cJSON_NULL		2
#define cJSON_Number	3
#define cJSON_String	4
#define cJSON_Array		5
#define cJSON_Object 	6

#define cJSON_IsReference	256
#define cJSON_StringIsConst	512

typedef struct cJSON{
	struct cJSON *next;
	struct cJSON *prev;
	struct cJSON *child;
	
	int type;

	char *valuestring;
	int valueint;
	double valuedouble;

	char *string;
} cJSON;

typedef struct cJSON_Hooks {
	void *(*malloc_fn) (size_t sz);
	void (*free_fn)(void *ptr);
} cJSON_Hooks;

// 提供malloc, realloc, free
extern void cJSON_InitHooks(cJSON_Hooks* hooks);
// 提供一个JSON块，这将返回一个您可以查询的cJSON对象。 完成后调用cJSON_Delete
extern cJSON *cJSON_Parse(const char *value);
// 将cJSON实体呈现为文本以进行传输/存储。 完成后释放字符*
extern char *cJSON_Print(cJSON *item);
// 将cJSON实体呈现为文本以进行传输/存储，而无需进行任何格式化。 完成后释放字符*
extern char *cJSON_PrintUnformatted(cJSON *item);
// 使用缓冲策略将cJSON实体呈现为文本。 prebuffer是对最终大小的猜测。 猜测得当可以减少重新分配。 fmt = 0表示未格式化，= 1表示格式化* /
extern char *cJSON_PrintBuffered(cJSON *item, int prebuffer, int fmt);
// 删除一个cJSON实体和所有子实体。
extern void cJSON_Delete(cJSON *c);
// 返回数组（或对象）中的项目数。
extern int cJSON_GetArraySize(cJSON *array);
// 从数组“ array”中检索项目编号“item”。 如果失败，则返回NULL。
extern cJSON *cJSON_GetArrayItem(cJSON *array, int item);
// 从对象获取项目“字符串”。 不区分大小写。
extern cJSON *cJSON_GetObjectItem(cJSON *object, const char *string); 
// 用于分析失败的分析。 这将返回一个指向解析错误的指针。 您可能需要回顾一些字符才能理解它。 在cJSON_Parse（）返回0时定义。当cJSON_Parse（）成功时为0。
extern const char *cJSOn_GetErrorPtr(void);
// 这些调用将创建适当类型的cJSON项
extern cJSON *cJSON_CreateNull(void);
extern cJSON *cJSON_CreateTrue(void);
extern cJSON *cJSON_CreateFalse(void);
extern cJSON *cJSON_CreateBool(int b);
extern cJSON *cJSON_CreateNumber(double num);
extern cJSON *cJSON_CreateString(const char *string);
extern cJSON *cJSON_CreateArray(void);
extern cJSON *cJSON_CreateObject(void);
// 这些实用程序创建一个计数项数组
extern cJSON *cJSON_CreateIntArray(const int  *numbers, int count);
extern cJSON *cJSON_CreateFloatArray(const float *numbers, int count);
extern cJSON *cJSON_CreateDoubleArray(const double *numbers, int count);
extern cJSON *cJSON_CreateStringArray(const char **strings, int count);
// 将项目追加到指定的数组/对象
extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
extern void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
// 当字符串绝对是const（即文字或一样好）并且一定会在cJSON对象中保留时使用此函数
extern void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);
// 将对项目的引用附加到指定的数组/对象。 当您要将现有的cJSON添加到新的cJSON，但又不想破坏现有的cJSON时，请使用此选项。
extern void cJSON_AddItemReferenceToArray(cJSON *arry, cJSON *item);
// 从数组/对象中删除/删除项目
extern cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);
extern void cJSON_DeleteItemFromArray(cJSON *array, int which);
extern cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
extern void cJSON_DeleteItemFromObject(cJSON *object, const char *string);
// 更新数组对象
// 将先前存在的项目向右移动
extern void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);
extern void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem);
// 复制一个cJSON项
/* Duplicate将在新的内存中创建与传递的cJSON项相同的新项，
需要释放。 如果recurse！= 0，它将复制与该项目相关的所有子项。
从Duplicate返回时，item-> next和-> prev指针始终为零。
*/
extern cJSON *cJSON_Duplicate(cJSON *item, int recurse);
// ParseWithOpts允许您要求（并检查）JSON是否终止为null，并检索指向已解析的最后字节的指针。
extern cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);
extern void cJSON_Minify(char *json);
// 快速创建的宏
#define cJSON_AddNullToObject(object, name) 	cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object, name) 	cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object, name)	cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object, name, b)		cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object, name, n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object, name, s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
// 分配整数值时，也需要将其传播为valuedouble
#define cJSON_SetIntValue(object, val)	((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
#define cJSON_SetNumberValue(object, val)	((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))

#ifdef __cplusplus
}
#endif

#endif