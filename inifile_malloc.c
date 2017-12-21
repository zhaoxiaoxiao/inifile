
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>

#include <fcntl.h>
#include <errno.h>

#include "inifile.h"

#define IS_SHOWDEBUG_MESSAGE		0

#if IS_SHOWDEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*ini file key value struct
*/
typedef struct key_value_node{
	char					*key;
	char					*value;
	struct key_value_node	*next;
}KEY_VALUE_NODE;
/*
*ini file section struct
*/
typedef struct file_section_node{
	char					*section;
	KEY_VALUE_NODE			*under;
	struct file_section_node *next;
}FILE_SECTION_NODE;
/*
*ini file info struct
*/
typedef struct ini_file_s{
	char					*name;
	FILE_SECTION_NODE		*under;
	struct ini_file_s		*next;
}INI_FILE_S;

static INI_FILE_S	*head = NULL,*tail = NULL;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void* malloc_memset(size_t size)
{
	void *p = NULL;

	p = malloc(size);
	if(p)
	{
		memset(p,0,size);
	}else{
		PERROR("system call malloc error\n");
	}
	
	return p;
}

void add_inifile_node(INI_FILE_S *p)
{
	if(head == NULL)
	{
		head = p;
		tail = p;
	}else{
		tail->next = p;
		tail = p;
	}
}

int check_inifile_isinit(const char *filename,int len,INI_FILE_S *q)
{
	int ret = 0;
	INI_FILE_S *p = head;

	do{
		if(p->name)
		{
			ret = memcmp((void *)(p->name),(void *)filename,len);
			if(ret == 0)
			{
				PERROR("The inifile is already load in memory\n");
				return INIFILE_ALREADY_INIT;
			}
		}else if(q == NULL){
			q = p;
		}

		p = p->next;
		if(q == NULL)
			ret++;
	}while(p);
	
	return ret;
}

int read_analys_ini_file(const char *file_name)
{
	int ret = 0;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};

	ret = access(file_name, R_OK|W_OK);
	if(ret != 0)
	{
		PERROR("File no exise of can't open\n");
		return INIFILE_NO_EXIST;
	}
	
	FILE *fp = fopen(file_name,"r");

	if(fp == NULL)
	{
		PERROR("File no exise of can't open\n");
		return INIFILE_NO_EXIST;
	}
	
	while(fgets(line,(INIFILE_MAX_CONTENT_LINELEN),fp) != NULL)
	{
		ret = init_every_line_inifile(ini_fd,line);
		if(ret < 0)
			break;
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	fclose(fp);
	return ret;	
}

void destroy_all_keyvalue_mem(FILE_SECTION_NODE *p)
{
	KEY_VALUE_NODE *pk = p->under,*qk = NULL;
	while(pk)
	{
		qk = pk->next;
		if(pk->key)
			free(pk->key);
		if(pk->value)
			free(pk->value);
		free(pk);
		pk = qk;
	}
	p->under = NULL;
}
void destroy_all_section_mem(INI_FILE_S *p)
{
	FILE_SECTION_NODE *pu = p->under,*qu = NULL;
	while(pu)
	{
		qu = pu->next;
		destroy_all_keyvalue_mem(pu);
		if(pu->section)
			free(pu->section);
		free(pu);
		pu = qu;
	}

	p->under = NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int init_ini_file(const char *filename,int len)
{
	char *file_name = NULL;
	int name_len = 0,ret = 0,result = 0,add = 0;
	INI_FILE_S *p_file = NULL;

	if(filename == NULL)
	{
		PERROR("parameter is null can't be use\n");
		return INIFILE_ERROR_PARAMETER;
	}

	if(len == 0)
		name_len = strlen(filename) + 1;
	else
		name_len = len + 1;
	len = name_len - 1;

	file_name = (char *)malloc_memset(name_len);
	if(file_name == NULL)
	{
		return INIFILE_SYSTEM_ERROR;
	}
	memcpy(file_name,filename,len);
	ret = access(file_name, 6);
	if(ret != 0)
	{
		PERROR("There is no power to access ini file\n");
		ret = INIFILE_POWER_ERROR;
		goto error_out;
	}

	if(head)
	{
		result = check_inifile_isinit(filename,len,&p_file);
		if(result < 0)
		{
			ret = result;
			goto error_out;
		}
	}
	if(p_file == NULL)
	{
		p_file = (INI_FILE_S *)malloc_memset(sizeof(INI_FILE_S));
		if(p_file == NULL)
		{
			ret = INIFILE_SYSTEM_ERROR;
			goto error_out;
		}
		add = 1;
	}

	ret = read_analys_ini_file(filename,p_file);
	if(ret < 0)
	{
		destroy_all_section_mem(p_file);
		goto error_out;
	}

	p_file->name = filename;
	if(add)
		add_inifile_node(p_file);
	return result;
error_out:
	if(file_name)
		free(file_name);
	if(p_file && add == 1)
		free(p_file);
	return ret;
}


void destroy_ini_source(int ini_fd)
{
	int i = 0;
	INI_FILE_S *p = head;
	if(p == NULL)
	{
		PERROR("There is no any inifile load in memory\n");
		return;
	}

	for(i = 0;i < ini_fd;i++)
	{
		p = p->next;
		if(p == NULL)
		{
			PERROR("The paramter is out of load inifile array bounds\n");
			return;
		}
	}

	destroy_all_section_mem(p);
	if(p->name)
		free(p->name);
	p->name = NULL;
	return;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_STRING_LEN 		64
#define MAX_FILENAME_LEN	64
#define MAX_FILLINE_LEN		128

typedef enum LineType{
	EMPTYLIST = -5,//
    WRITEERROR = -4,//д�ļ����ش���
	REPEATNODEKEY = -3,//�ڵ���߽ڵ��¼�ֵ�ظ�
    OPENFILEERROR = -2,//���ļ�����
    TOOLONG = -1,//������̫��
    ERRORTYPR = 0,//�Ȳ��ǽڵ㣬Ҳ���ǽڵ㣬�������е���һ��ֱ�ӱ�����
    NODETYPE = 1,//�ڵ�
    KEYVALUE = 2,//��ֵ
    NOTESTYPE = 3,//ע�ͣ�ע�Ϳ�����Ϊ�ڵ�洢Ҳ������Ϊ�ڵ��¼�ֵ�洢
}LineType;

typedef enum method_type{
	initIniFile_type,
	getValueOfKey_type,
	updateValueOfKey_type,
	addValueOfKey_type,
	deleteValueOfKey_type,
	deleteSection_type,
	addSetction_type,
	exitOperationIniFile_type,
}METHOD_TYPE;

typedef struct KeyValueNode{
    char key[(MAX_STRING_LEN+4)];
    char value[(MAX_STRING_LEN+4)];
    int isNote;//�Ƿ���ע�ͣ�1��ע�ͣ�Ĭ����0������ע��
    struct KeyValueNode *pre;
    struct KeyValueNode *next;
}KeyValueNode;

//ini�ļ���ֵ�ڵ������ṹ��
typedef struct ListkeyValueNode{
    KeyValueNode *head;
    KeyValueNode *tail;
    int len;
}ListkeyValueNode;

//ini�ļ��нڵ�ڵ�ṹ��
typedef struct IniFileNode{
    char section[(MAX_STRING_LEN+4)];
    int isNote;//�Ƿ���ע�ͣ�1��ע�ͣ�Ĭ����0������ע��
    ListkeyValueNode *listkeyValueNode;
    struct IniFileNode *pre;
    struct IniFileNode *next;
}IniFileNode;

//ini�ļ��нڵ㹹�ɵ�����
typedef struct ini_file{
    IniFileNode *head;
    IniFileNode *tail;
    char fileName[MAX_FILENAME_LEN];
    int len;
}INI_FILE;


//ini文件节点链表初始化
void listIniFileNode_init(INI_FILE *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
	memset((void *)list->fileName,0,MAX_FILENAME_LEN);
}

//ini文件中键值链表初始化
void listkeyValueNode_init(ListkeyValueNode *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

//判断节点链表是够为空，false为空，true为非空
bool listIniFileNode_is_empty(INI_FILE *list)
{
    if(list->head == NULL)
        return false;
    else
        return true;
}

//判断键值链表是够为空，false为空，true为非空
bool listkeyValueNode_is_empty(ListkeyValueNode *list)
{
    if(list->head == NULL)
        return false;
    else
        return true;
}

//ini文件节点链表上插入一个节点
void listIniFileNode_insert(INI_FILE *list,IniFileNode *node)
{

	if(list == NULL || node == NULL)
		return;

    if(list->head == NULL)
	{
        list->head = node;
		list->tail = node;
		node->pre = NULL;
		node->next = NULL;
		list->len++;
    }else
    {
    	node->pre = list->tail;
		list->tail->next = node;
    	list->tail = node;
    	node->next = NULL;
    	list->len++;
	}
}

//键值链表尾部插入一个节点
void listkeyValueNode_insert(ListkeyValueNode *list,KeyValueNode *node)
{

	if(node == NULL || list == NULL)
		return;

    if(list->head == NULL)
    {
        list->head = node;
		list->tail = node;
		node->pre = NULL;
		node->next = NULL;
		list->len++;
    }else
    {
    	node->pre = list->tail;
		list->tail->next = node;
    	list->tail = node;
    	node->next = NULL;
    	list->len++;
    }
}

//删除一个键值对
void listkeyValueNode_delete(ListkeyValueNode *list,KeyValueNode **node)
{
	KeyValueNode *p = NULL;
	KeyValueNode *p_node = *node;

	if(list == NULL || p_node == NULL)
		return;

	p = list->head;
	
	while(p != NULL)
	{
		if(p == p_node)
			break;
		else
			p = p->next;
	}

	if(p == NULL)
		return;

    if((list->head == list->tail) && (list->head != NULL))
    {
    	free(p_node);
    	list->head = NULL;
		list->tail = NULL;
		p_node = NULL;
		*node = NULL;
		return;
    }else
    {
        if(p_node->pre != NULL)
        {
            p_node->pre->next = p_node->next;
        }else
        {
            list->head = p_node->next;
			list->head->pre = NULL;
        }

        if(p_node->next != NULL)
        {
            p_node->next->pre = p_node->pre;
        }else
        {
            list->tail = p_node->pre;
			list->tail->next = NULL;
        }
    }
    free(p_node);
	p_node = NULL;
	*node = NULL;
	return;
}

//删除一个节点所有键值，键值链表置空
void listkeyValueNode_deleteall(ListkeyValueNode **list)
{
    KeyValueNode *node = NULL;
	ListkeyValueNode *p_list = *list;

	if(p_list == NULL)
		return;

    if(!listkeyValueNode_is_empty(p_list))
    {
    	free(p_list);
		p_list = NULL;
		*list = NULL;
        return;
    }

    while(p_list->head && (p_list->head != p_list->tail))
    {
    	PDEBUG("free key value\n");
        node = p_list->tail;
        p_list->tail = node->pre;
        free(node);
    }
	PDEBUG("free key value\n");
    node = p_list->head;
    free(node);
	free(p_list);
	p_list = NULL;
	*list = NULL;
	return;
}

//删除ini文件节点链表上一个节点
void listIniFileNode_delete(INI_FILE *list,IniFileNode *node)
{
	IniFileNode *p = NULL;

	if(list == NULL || node == NULL)
		return;

	p = list->head;
	while(p != NULL)
	{
		if(p == node)
			break;
		else
			p = p->next;
	}
	if(p == NULL)
		return;

    if(list->head == list->tail)
    {
    	free(node);
		return;
    }else
    {
        if(node->pre != NULL)
        {
            node->pre->next = node->next;
        }else
        {
            list->head = node->next;
			list->head->pre = NULL;
        }

        if(node->next != NULL)
        {
            node->next->pre = node->pre;
        }else
        {
            list->tail = node->pre;
			list->tail->next = NULL;
        }
    }
    listkeyValueNode_deleteall(&(node->listkeyValueNode));
    free(node);
    return;
}

//删除一个字符串数据中的空白 tab字符
char *delete_spacetab_begin(char *line){
    char *p = line;
	if(*p == CONTENT_NEELINE_SIGN)
		return NULL;
	
    while(*p == CONTENT_SPACE_SIGN || *p == CONTENT_TAB_SIGN){
        p++;
        if(*p == CONTENT_NULL){
            return NULL;
        }
    }
	return p;
}

//判断行类容中有没有‘=’来判断是不是键值行
int isKeyValueLine(char *line)
{
    char *p = line;
    int index = 0;
    while(*p != CONTENT_EQUALITY_SIGN)
    {
        p++;
        index++;
        if(*p == 0)
        {
            return 0;
        }
    }
    return index;
}

//从源字符串中指定位置区间拷贝到目标字符串中，第begin个字符拷贝，第end个字符不拷贝，
//区间大小已经在之前验证过了，此方法不考虑越界情况
void copyBeginEnd(char *dest,char *sort,int begin,int end)
{
    char *p_char = sort + begin;
	int len = end - begin;
	memcpy((void *)dest,(void *)p_char,len);
}

//查找节点下键值
KeyValueNode *findIniKeyValue(IniFileNode *iniFileNode,char *key)
{
	ListkeyValueNode *pListkeyValueNode = NULL;
	KeyValueNode *pKeyValueNode = NULL;
	char *p_char = NULL;
	int com_len = 0,len = strlen(key);
	
	if(iniFileNode == NULL || key == NULL)
		return NULL;

	if(iniFileNode ->listkeyValueNode == NULL)
		return NULL;

	pListkeyValueNode = iniFileNode ->listkeyValueNode;

	if(!listkeyValueNode_is_empty(pListkeyValueNode))
		return NULL;

	pKeyValueNode = pListkeyValueNode->head;
	while(pKeyValueNode != NULL)
	{
		if(memcmp((void *)(pKeyValueNode->key),(void *)key,len) == 0)
		{
			com_len = strlen(pKeyValueNode->key);
			if(com_len == len)
				return pKeyValueNode;
			if(com_len > len)
			{
				p_char = pKeyValueNode->key + len;
				while(com_len > len)
				{
					if(*p_char != CONTENT_SPACE_SIGN && *p_char != CONTENT_TAB_SIGN)
					{
						break;
					}
					com_len--;
					p_char++;
				}
				if(com_len == len)
					return pKeyValueNode;
			}	
		}	
		pKeyValueNode = pKeyValueNode->next;
	}
	return pKeyValueNode;
}

//查找节点，
IniFileNode *findIniFileNode(INI_FILE *p_inifile,char *section)
{
	IniFileNode *p = NULL;
	char *p_char = NULL;
	int com_len = 0 ,len = strlen(section);
	if(p_inifile == NULL || section == NULL)
		return NULL;
	if(!listIniFileNode_is_empty(p_inifile))
		return NULL;
	p = p_inifile->head;
	while(p != NULL)
	{
		if(memcmp((void*)(p->section),(void *)section,len) == 0)
		{
			com_len = strlen(p->section);
			if(com_len == len)
				return p;
			if(com_len > len)
			{
				p_char = p->section + len;
				while(com_len > len)
				{
					if(*p_char != CONTENT_SPACE_SIGN && *p_char != CONTENT_TAB_SIGN)
					{
						break;
					}
					com_len--;
					p_char++;
				}
				if(com_len == len)
					return p;
			}	
		}	
		p = p->next;
	}

	return p;
}


//如果这一行是节点，那就初始化一个节点，
int initLineNode(INI_FILE *p_inifile,char *line)
{
	IniFileNode *pIniFileNode = (IniFileNode *)malloc(sizeof(IniFileNode));
	memset(pIniFileNode,0,sizeof(IniFileNode));
	pIniFileNode->isNote = 0;
	copyBeginEnd(pIniFileNode->section,line,1,strlen(line)-2);
	if(findIniFileNode(p_inifile,pIniFileNode->section) != NULL)
	{
		free(pIniFileNode);
		return REPEATNODEKEY;
	}
	listIniFileNode_insert(p_inifile,pIniFileNode);
	return 0;
}

//如果这一行是一行注释，就初始化一行注释，注释分两种情况，一种是初始化一个节点，一种是初始化一个键值点
void initLineNotes(INI_FILE *p_inifile,char *line)
{
	if(p_inifile->tail == NULL)
	{
		IniFileNode *pIniFileNode = (IniFileNode *)malloc(sizeof(IniFileNode));
		memset(pIniFileNode,0,sizeof(IniFileNode));
		pIniFileNode->isNote = 1;
		copyBeginEnd(pIniFileNode->section,line,1,strlen(line));
		listIniFileNode_insert(p_inifile,pIniFileNode);
	}else if(p_inifile->tail->isNote == 1)
	{
	    IniFileNode *pIniFileNode = (IniFileNode *)malloc(sizeof(IniFileNode));
		memset(pIniFileNode,0,sizeof(IniFileNode));
		pIniFileNode->isNote = 1;
		copyBeginEnd(pIniFileNode->section,line,1,strlen(line));
		listIniFileNode_insert(p_inifile,pIniFileNode);
	}else
	{
		if(p_inifile->tail->listkeyValueNode == NULL)
		{
			p_inifile->tail->listkeyValueNode = (ListkeyValueNode *)malloc(sizeof(ListkeyValueNode));
			listkeyValueNode_init(p_inifile->tail->listkeyValueNode);
			KeyValueNode *pKeyValueNode = (KeyValueNode *)malloc(sizeof(KeyValueNode));
			memset(pKeyValueNode,0,sizeof(KeyValueNode));
			pKeyValueNode->isNote = 1;
			copyBeginEnd(pKeyValueNode->key,line,1,strlen(line));
			listkeyValueNode_insert(p_inifile->tail->listkeyValueNode,pKeyValueNode);
		}else
		{
			KeyValueNode *pKeyValueNode = (KeyValueNode *)malloc(sizeof(KeyValueNode));
			memset(pKeyValueNode,0,sizeof(KeyValueNode));
			pKeyValueNode->isNote = 1;
			copyBeginEnd(pKeyValueNode->key,line,1,strlen(line));
			listkeyValueNode_insert(p_inifile->tail->listkeyValueNode,pKeyValueNode);
		}
	}
}

//如果这一行是键值，初始化一个键值，分几种情况：
//一种是没有节点，直接忽略这一行，
//一种有节点，但是都是注释节点，也忽略
//最后一种就是有真正的节点，添加键值
//参数index是检测行中是否含有‘=’返回的索引值
int initLineKeyValue(INI_FILE *p_inifile,char *line,int index)
{
	if(p_inifile->tail == NULL)
	{
		return 0;
	}else
	{
		if(p_inifile->tail->isNote == 1)
		{
			return 0;
		}else
		{
			if(p_inifile->tail->listkeyValueNode == NULL)
			{
				p_inifile->tail->listkeyValueNode = (ListkeyValueNode *)malloc(sizeof(ListkeyValueNode));
				listkeyValueNode_init(p_inifile->tail->listkeyValueNode);
				KeyValueNode *pKeyValueNode = (KeyValueNode *)malloc(sizeof(KeyValueNode));
				memset(pKeyValueNode,0,sizeof(KeyValueNode));
				pKeyValueNode->isNote = 0;
				copyBeginEnd(pKeyValueNode->key,line,0,index);
				copyBeginEnd(pKeyValueNode->value,line,index+1,strlen(line)-1);
				listkeyValueNode_insert(p_inifile->tail->listkeyValueNode,pKeyValueNode);
			}else
			{
				KeyValueNode *pKeyValueNode = (KeyValueNode *)malloc(sizeof(KeyValueNode));
				memset(pKeyValueNode,0,sizeof(KeyValueNode));
				pKeyValueNode->isNote = 0;
				copyBeginEnd(pKeyValueNode->key,line,0,index);
				if(findIniKeyValue(p_inifile->tail,pKeyValueNode->key)!=NULL)
				{
					free(pKeyValueNode);
					return REPEATNODEKEY;
				}
				copyBeginEnd(pKeyValueNode->value,line,index+1,strlen(line)-1);
				listkeyValueNode_insert(p_inifile->tail->listkeyValueNode,pKeyValueNode);
			}
		}
	}
	return 0;
}


//对每一行读到的类型进行分类，节点的初始为节点元素，键值初始化为节点下键值元素，
//返回值为LineType
 int initLineContext(INI_FILE *p_inifile,char *line)
 {
    int index = 0;//
    char *p_char = NULL;
    p_char = delete_spacetab_begin(line);

	if(p_char == NULL)
		return ERRORTYPR;
	
    if(*p_char == CONTENT_NEELINE_SIGN)
        return ERRORTYPR;
	
    if(*p_char == CONTENT_LEFT_BRACKET)
    {
        //节点
        if(strlen(p_char) > (MAX_STRING_LEN+2))
            return TOOLONG;
		
        index = initLineNode(p_inifile,p_char);
		if(index < 0)
			return index;
        return NODETYPE;
    }

    if(*line == CONTENT_COMMA_SIGN)
    {
        //注释
        if(strlen(line) > MAX_STRING_LEN)
            return TOOLONG;
        initLineNotes(p_inifile,line);
        return NOTESTYPE;
    }

	index = isKeyValueLine(line);
    if(index > 0)//判断是否有=
    {
        //键值
        if(strlen(line) > MAX_FILLINE_LEN)
            return TOOLONG;
        index = initLineKeyValue(p_inifile,line,index);
		if(index < 0)
			return index;
        return KEYVALUE;
    }
	
    return ERRORTYPR;
 }


//读取文件类容，一行一行的读取，一直读到文件结束，换行符ascii码是10，空格键是32，制表符是9
//如果有一行长度大于指定长度，返回长度错误，会导致整个程序出错的，
int readIniFile(INI_FILE *p_inifile,const char *filename){
    char line[(MAX_FILLINE_LEN+4)] = {0};
    int result = 0;
    FILE *fp = fopen(filename,"a+");
	
    if(fp == NULL){
		fprintf(stderr, "---%s--%d read file error,can't open file:%s---\n", __FUNCTION__, __LINE__, strerror(errno));
		return  OPENFILEERROR;
	}
	while(fgets(line,(MAX_FILLINE_LEN),fp) != NULL){
        result = initLineContext(p_inifile,line);
		if(result < 0)
		{
			fprintf(stderr, "---%s--%d---line:%s\n", __FUNCTION__, __LINE__, line);
		    fclose(fp);
			return result;//这个错误会导致ini文件初始化失败
		}
		memset(line,0,(MAX_FILLINE_LEN+4));
	}
	fclose(fp);
	return 0;
}

//释放ini文件所有的节点内存
void destoryListIniFileNode(INI_FILE *p_inifile)
{
    IniFileNode *iniFileNode = NULL;

	if(p_inifile == NULL)
		return;

	if(!listIniFileNode_is_empty(p_inifile))
	{
		free(p_inifile);
		return;
	}

    while(p_inifile->tail != p_inifile->head)
    {
    	PDEBUG("free node\n");
		iniFileNode = p_inifile->tail;
		p_inifile->tail = iniFileNode->pre;
		listkeyValueNode_deleteall(&(iniFileNode->listkeyValueNode));
		free(iniFileNode);
    }
	PDEBUG("free node\n");
	iniFileNode = p_inifile->head;
	listkeyValueNode_deleteall(&(iniFileNode->listkeyValueNode));
	free(iniFileNode);
	free(p_inifile);
	return;

}

//根据ini文件在内存中操作的链表头指针重新更新ini文件，重新写文件
int updateIniFile(INI_FILE *p_inifile)
{
	int file_fd;//打开文件描述符
	int bytes_write,buffersSize;//写字节大小，应该写字节大小
	char *ptr;//指向写缓冲区的指针
	int errno;//错误描述变量
	IniFileNode *p = NULL;//节点指针
	KeyValueNode *q = NULL;//键值指针
	char buffer[(MAX_FILLINE_LEN+4)];//写缓冲区
	if(!listIniFileNode_is_empty(p_inifile))
		return EMPTYLIST;
	//remove(p_inifile->fileName);
	if((file_fd=open(p_inifile->fileName,O_RDWR|O_TRUNC))==-1)
	{
		fprintf(stderr, "---%s--%d open file error:%s---\n", __FUNCTION__, __LINE__, strerror(errno));
		return OPENFILEERROR;
	}
    p = p_inifile ->head;
	while(p != NULL)
	{
		memset(buffer,0,(MAX_FILLINE_LEN+4));
		strcpy(buffer,"\n");
		if(p ->isNote == 1)
		{
			strcat(buffer,";");
			strcat(buffer,p->section);
			strcat(buffer,"\n");
		}

		if(p ->isNote == 0)
		{
			strcat(buffer,"[");
			strcat(buffer,p->section);
			strcat(buffer,"]");
			strcat(buffer,"\n");
		}

		buffersSize = strlen(buffer);
		ptr=buffer;
		while(bytes_write=write(file_fd,ptr,buffersSize))
		{
			if((bytes_write==-1)&&(errno!=EINTR))
				return WRITEERROR;
			else if(bytes_write==buffersSize)
				break;
			else if(bytes_write>0)
			{
				ptr+=bytes_write;
				buffersSize-=bytes_write;
			}
		}

		if(bytes_write==-1)
			return WRITEERROR;

		if(p->listkeyValueNode != NULL)
		{
			q = p->listkeyValueNode->head;
			while(q != NULL)
			{
				memset(buffer,0,(MAX_FILLINE_LEN+4));
				if(q->isNote == 1)
				{
					strcpy(buffer,";");
					strcat(buffer,q->key);
					strcat(buffer,"\n");
				}

				if(q->isNote == 0)
				{
					strcpy(buffer,q->key);
					strcat(buffer,"=");
					strcat(buffer,q->value);
					strcat(buffer,"\n");
				}

				buffersSize = strlen(buffer);
				ptr=buffer;
				while(bytes_write=write(file_fd,ptr,buffersSize))
				{
					if((bytes_write==-1)&&(errno!=EINTR))
						return WRITEERROR;
					else if(bytes_write==buffersSize)
						break;
					else if(bytes_write>0)
					{
						ptr+=bytes_write;
						buffersSize-=bytes_write;
					}
				}

			if(bytes_write==-1)
				return WRITEERROR;
			q = q->next;
			}
		}
		p = p->next;
	}
	close(file_fd);
	return 0;
}

bool ini_parameter_check(INI_PARAMETER *parameter,METHOD_TYPE type)
{
	char *p_char = NULL;
	if(type == getValueOfKey_type)
	{
		if(parameter->section == NULL || parameter->key == NULL || parameter->section_len <= 0 || parameter->section_len >MAX_STRING_LEN
			|| parameter->key_len <= 0 || parameter->key_len > MAX_STRING_LEN )
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else if(type == updateValueOfKey_type)
	{
		if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL || parameter->section_len <= 0 || parameter->section_len > MAX_STRING_LEN
			|| parameter->key_len <= 0 || parameter->key_len > MAX_STRING_LEN || parameter->value_len <= 0 || parameter->value_len > MAX_STRING_LEN)
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->value + parameter->value_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else if(type == addValueOfKey_type)
	{
		if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL ||parameter->section_len <= 0 || parameter->section_len > MAX_STRING_LEN
			|| parameter->key_len <= 0 || parameter->key_len > MAX_STRING_LEN || parameter->value_len <= 0 || parameter->value_len > MAX_STRING_LEN)
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->value + parameter->value_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else if(type == deleteValueOfKey_type)
	{
		if(parameter->section == NULL || parameter->key == NULL ||parameter->section_len <= 0 || parameter->section_len > MAX_STRING_LEN
			|| parameter->key_len <= 0 || parameter->key_len > MAX_STRING_LEN)
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else if(type == deleteSection_type)
	{
		if(parameter->section == NULL || parameter->section_len <= 0 || parameter->section_len > MAX_STRING_LEN)
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else if(type == addSetction_type)
	{
		if(parameter->section == NULL || parameter->section_len <= 0 || parameter->section_len > MAX_STRING_LEN)
		{
			return false;
		}
		else
		{
			p_char = parameter->section + parameter->section_len;
			if(*p_char != '\0')
			{
				return false;
			}
			p_char = parameter->key + parameter->key_len;
			if(*p_char != '\0')
			{
				return false;
			}
		}
	}else
	{
		printf("unknow function type\n");
		return false;
	}
	return true;
}



/* *
 * *  @brief       	初始化一个ini文件
 * *  @author      	zhaoxiaoxiao
 * *  @date        	2014-06-21
 * *  @fileName   	文件路径
 * *  @return      	ini文件在内存中操作的链表头指针，空说明初始化失败，文件不存在或者文件打开失败，或者文件中某一行内容超过指定长度
 * */
INI_FILE *initIniFile(const char *fileName,int len)
{
	
	int result = 0;
	const char *p_char = NULL;
	INI_FILE *p_inifile = NULL;
	
	if(fileName == NULL || len > MAX_FILENAME_LEN || len <= 0)
		return NULL;
	
	p_char = fileName + len;
	
	if(*p_char != '\0')
	{
		return NULL;
	}

	result = access(fileName, 6);
	if(result != 0)
	{
		fprintf(stderr, "---%s--%d FILE not exist:%s---\n", __FUNCTION__, __LINE__, strerror(errno));
		return NULL;
	}
	p_inifile = (INI_FILE *)malloc(sizeof(INI_FILE));
	listIniFileNode_init(p_inifile);
	memcpy((void *)p_inifile->fileName,(void *)fileName,len);
    result = readIniFile(p_inifile,fileName);
	if(result <0 )
	{
		fprintf(stderr, "---%s--%d readIniFile error:%s---\n", __FUNCTION__, __LINE__, strerror(errno));
		return NULL;
	}
	else
		return p_inifile;
}

/* *
 * *  @brief       	                通过建获取ini文件值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针,
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                值指针；value will be lost after exitOperationIniFile
 * */
char *getValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
	IniFileNode * p = NULL;
	KeyValueNode *q = NULL;
	char *p_char = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,getValueOfKey_type))
		return NULL;
	
	p = findIniFileNode(p_inifile,parameter->section);
	if(p == NULL)
		return NULL;
	q = findIniKeyValue(p,parameter->key);
	
	if(q)
	{
		p_char = delete_spacetab_begin(q->value);
		return p_char;
	}
	else
		return NULL;
}

/* *
 * *  @brief       	                修改一个ini文件键值，只能修改值，不能修改键
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，其他失败，-1参数错误，-2节点没有找到，-3键值没有找到
 * */
int updateValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
	IniFileNode * p = NULL;
	KeyValueNode *q = NULL;
	char *p_char = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,updateValueOfKey_type))
		return -1;

	p = findIniFileNode(p_inifile,parameter->section);
	if(p == NULL)
		return -2;
	q = findIniKeyValue(p,parameter->key);
	if(q == NULL)
		return -3;
	memset(q->value, 0, MAX_STRING_LEN);
	p_char = q->value;
	*p_char = CONTENT_SPACE_SIGN;
	p_char++;
	memcpy((void *)p_char,(void *)parameter->value,parameter->value_len);
	p_char = p_char + parameter->value_len;
	*p_char = CONTENT_NEELINE_SIGN;
    updateIniFile(p_inifile);
	return 0;
}

/* *
 * *  @brief       	                增加一个ini文件键值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，-3是已存在键值
 * */
int addValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
	IniFileNode * p = NULL;
	KeyValueNode *q = NULL;
	char *p_char = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,addValueOfKey_type))
		return -1;
	
	p = findIniFileNode(p_inifile,parameter->section);
	if(p == NULL)
		return -2;
	q = findIniKeyValue(p,parameter->key);
	
	if(q != NULL)
		return -3;
	
	q = (KeyValueNode *)malloc(sizeof(KeyValueNode));
	memset(q,0,sizeof(KeyValueNode));
	memcpy((void *)q->key,(void *)parameter->key,parameter->key_len);
	p_char = q->key + parameter->key_len;
	*p_char = CONTENT_TAB_SIGN;

	p_char = q->value;
	*p_char = CONTENT_SPACE_SIGN;
	p_char++;
	memcpy((void *)p_char,(void *)parameter->value,parameter->value_len);
	p_char = p_char + parameter->value_len;
	*p_char = CONTENT_NEELINE_SIGN;
	if(p->listkeyValueNode == NULL)
	{
		p->listkeyValueNode = (ListkeyValueNode *)malloc(sizeof(ListkeyValueNode));
		listkeyValueNode_init(p->listkeyValueNode);
	}
	listkeyValueNode_insert(p->listkeyValueNode,q);
	updateIniFile(p_inifile);
    return 0;
}

/* *
 * *  @brief       	                删除一个ini文件键值
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter					节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，-3是没有找到键值
 * */
int deleteValueOfKey(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
    IniFileNode * p = NULL;
	KeyValueNode *q = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,deleteValueOfKey_type))
		return -1;

	p = findIniFileNode(p_inifile,parameter->section);
	if(p == NULL)
		return -2;

	q = findIniKeyValue(p,parameter->key);
	if(q == NULL)
		return -3;
	
	listkeyValueNode_delete(p->listkeyValueNode,&q);
	updateIniFile(p_inifile);
    return 0;
}

/* *
 * *  @brief       	                删除一个ini文件节点
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter					节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点没有找到，
 * */
int deleteSection(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
    IniFileNode * p = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,deleteSection_type))
		return -1;
	
	p = findIniFileNode(p_inifile,parameter->section);
	if(p == NULL)
		return -2;
	listIniFileNode_delete(p_inifile,p);
	updateIniFile(p_inifile);
    return 0;
}

/* *
 * *  @brief       	                增加一个ini文件节点
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @parameter                    节点名称,参数结构体，详见结构体描述
 * *  @return      	                0操作成功，-1没有参数错误，-2节点已经存在
 * */
int addSetction(INI_FILE *p_inifile,INI_PARAMETER *parameter)
{
	IniFileNode * p = NULL;

	if(!p_inifile || !ini_parameter_check(parameter,addSetction_type))
		return -1;
	/*
	if(!listIniFileNode_is_empty(p_inifile))
		return -1;
	*/
	
	p = findIniFileNode(p_inifile,parameter->section);
	if(p != NULL)
		return -2;

	IniFileNode *pIniFileNode = (IniFileNode *)malloc(sizeof(IniFileNode));
	memset(pIniFileNode,0,sizeof(IniFileNode));
	pIniFileNode->isNote = 0;
	copyBeginEnd(pIniFileNode->section,parameter->section,0,parameter->section_len);
	if(findIniFileNode(p_inifile,pIniFileNode->section) != NULL)
	{
		free(pIniFileNode);
		return -1;
	}
	listIniFileNode_insert(p_inifile,pIniFileNode);

	updateIniFile(p_inifile);
    return 0;
}

/* *
 * *  @brief       	                清除一个ini文件所有操作，释放所有内存
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini文件在内存中操作的链表头指针
 * *  @return      	                0操作成功，-1参数错误
 * */
int exitOperationIniFile(INI_FILE **p_inifile)
{
	if(*p_inifile == NULL)
		return -1;
	
	PDEBUG("free enter\n");
	destoryListIniFileNode(*p_inifile);
	
	*p_inifile = NULL;
	return 0;
}

