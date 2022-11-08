#pragma once
#define FILE_MAX_LEN 1473
struct file_opt
{
	char type;
	char filename[20];
	char mess[FILE_MAX_LEN];
};