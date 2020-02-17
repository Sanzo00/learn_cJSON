/*
	格式化显示得到的json数据
*/

#include <stdio.h>
#include <string.h>
#include "cJSON.h"

int main() {

	FILE* fp = fopen("doc.txt", "r");
	cJSON* json = NULL;
	char* out;
	char line[1024] = { 0 };

	if (fp) {
		out = line;
		while (fgets(out, sizeof(out), fp)) {
			out += strlen(out);
		}
		printf("%s\n", line);

		json = cJSON_Parse(line);
		out = cJSON_Print(json);
		// 格式化输出json
		printf("%s\n", out);
		// 获取对象
		cJSON* arrayItem = cJSON_GetObjectItem(json, "info");
		// 获取数组中第一个成员
		cJSON* object = cJSON_GetArrayItem(arrayItem, 0);

		cJSON* item = cJSON_GetObjectItem(object, "user");
		printf("user: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "password");
		printf("password: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "type");
		printf("type: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "ip");
		printf("ip: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "port");
		printf("dp_port: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "protocol");
		printf("protocol: %s\n", item->valuestring);

		// 获取数组中第2个成员
		object = cJSON_GetArrayItem(arrayItem, 1);

		item = cJSON_GetObjectItem(object, "sex");
		printf("sex: %s\n", item->valuestring);
		item = cJSON_GetObjectItem(object, "age");
		printf("age: %lf\n", item->valuedouble);

		fclose(fp);
		cJSON_Delete(json);
	}
	else {
		printf("fopen error\n");
	}

	return 0;
}