/*
	自定义数据成员结构, 得到对应的json
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int main() {

	cJSON* root = cJSON_CreateObject();
	cJSON* p1 = cJSON_CreateArray();
	cJSON* p2 = cJSON_CreateObject();
	cJSON* p3 = cJSON_CreateObject();
	
	cJSON_AddItemToObject(root, "info", p1);
	cJSON_AddItemToArray(p1, p2);
	cJSON_AddItemToArray(p1, p3);
	// 添加成员到p2
	cJSON_AddStringToObject(p2, "user", "sanzo");
	cJSON_AddStringToObject(p2, "password", "123456");
	cJSON_AddStringToObject(p2, "type", "coder");
	cJSON_AddStringToObject(p2, "ip", "192.168.1.101");
	cJSON_AddStringToObject(p2, "port", "6666");
	cJSON_AddStringToObject(p2, "protocol", "tcp/udp");
	// 添加成员到p3
	cJSON_AddStringToObject(p3, "sex", "male");
	cJSON_AddNumberToObject(p3, "age", 20);
	// 整型
	cJSON_AddNumberToObject(root, "number", 2.14);
	// 整形数组
	int arr[6] = { 1, 2, 3, 4, 5, 6};
	cJSON* p4 = cJSON_CreateIntArray(arr, 6);
	cJSON_AddItemToObject(root, "arr", p4);

	char* out = cJSON_Print(root);
	printf("%s\n", out);

	free((void*)out);
	cJSON_Delete(root);

	return 0;
}