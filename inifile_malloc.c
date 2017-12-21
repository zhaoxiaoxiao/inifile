
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
    WRITEERROR = -4,//写文件严重错误
	REPEATNODEKEY = -3,//节点或者节点下键值重复
    OPENFILEERROR = -2,//打开文件错误
    TOOLONG = -1,//行内容太长
    ERRORTYPR = 0,//既不是节点，也不是节点，在文章中的这一行直接被忽略
    NODETYPE = 1,//节点
    KEYVALUE = 2,//键值
    NOTESTYPE = 3,//注释，注释可以作为节点存储也可以作为节点下键值存储
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
    int isNote;//是否是注释，1是注释，默认是0，不是注释
    struct KeyValueNode *pre;
    struct KeyValueNode *next;
}KeyValueNode;

//ini文件键值节点链表结构体
typedef struct ListkeyValueNode{
    KeyValueNode *head;
    KeyValueNode *tail;
    int len;
}ListkeyValueNode;

//ini文件中节点节点结构体
typedef struct IniFileNode{
    char section[(MAX_STRING_LEN+4)];
    int isNote;//是否是注释，1是注释，默认是0，不是注释
    ListkeyValueNode *listkeyValueNode;
    struct IniFileNode *pre;
    struct IniFileNode *next;
}IniFileNode;

//ini文件中节点构成的链表
typedef struct ini_file{
    IniFileNode *head;
    IniFileNode *tail;
    char fileName[MAX_FILENAME_LEN];
    int len;
}INI_FILE;


//ini鏂囦欢鑺傜偣閾捐〃鍒濆鍖�
void listIniFileNode_init(INI_FILE *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
	memset((void *)list->fileName,0,MAX_FILENAME_LEN);
}

//ini鏂囦欢涓敭鍊奸摼琛ㄥ垵濮嬪寲
void listkeyValueNode_init(ListkeyValueNode *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;
}

//鍒ゆ柇鑺傜偣閾捐〃鏄涓虹┖锛宖alse涓虹┖锛宼rue涓洪潪绌�
bool listIniFileNode_is_empty(INI_FILE *list)
{
    if(list->head == NULL)
        return false;
    else
        return true;
}

//鍒ゆ柇閿�奸摼琛ㄦ槸澶熶负绌猴紝false涓虹┖锛宼rue涓洪潪绌�
bool listkeyValueNode_is_empty(ListkeyValueNode *list)
{
    if(list->head == NULL)
        return false;
    else
        return true;
}

//ini鏂囦欢鑺傜偣閾捐〃涓婃彃鍏ヤ竴涓妭鐐�
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

//閿�奸摼琛ㄥ熬閮ㄦ彃鍏ヤ竴涓妭鐐�
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

//鍒犻櫎涓�涓敭鍊煎
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

//鍒犻櫎涓�涓妭鐐规墍鏈夐敭鍊硷紝閿�奸摼琛ㄧ疆绌�
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

//鍒犻櫎ini鏂囦欢鑺傜偣閾捐〃涓婁竴涓妭鐐�
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

//鍒犻櫎涓�涓瓧绗︿覆鏁版嵁涓殑绌虹櫧 tab瀛楃
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

//鍒ゆ柇琛岀被瀹逛腑鏈夋病鏈夆��=鈥欐潵鍒ゆ柇鏄笉鏄敭鍊艰
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

//浠庢簮瀛楃涓蹭腑鎸囧畾浣嶇疆鍖洪棿鎷疯礉鍒扮洰鏍囧瓧绗︿覆涓紝绗琤egin涓瓧绗︽嫹璐濓紝绗琫nd涓瓧绗︿笉鎷疯礉锛�
//鍖洪棿澶у皬宸茬粡鍦ㄤ箣鍓嶉獙璇佽繃浜嗭紝姝ゆ柟娉曚笉鑰冭檻瓒婄晫鎯呭喌
void copyBeginEnd(char *dest,char *sort,int begin,int end)
{
    char *p_char = sort + begin;
	int len = end - begin;
	memcpy((void *)dest,(void *)p_char,len);
}

//鏌ユ壘鑺傜偣涓嬮敭鍊�
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

//鏌ユ壘鑺傜偣锛�
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


//濡傛灉杩欎竴琛屾槸鑺傜偣锛岄偅灏卞垵濮嬪寲涓�涓妭鐐癸紝
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

//濡傛灉杩欎竴琛屾槸涓�琛屾敞閲婏紝灏卞垵濮嬪寲涓�琛屾敞閲婏紝娉ㄩ噴鍒嗕袱绉嶆儏鍐碉紝涓�绉嶆槸鍒濆鍖栦竴涓妭鐐癸紝涓�绉嶆槸鍒濆鍖栦竴涓敭鍊肩偣
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

//濡傛灉杩欎竴琛屾槸閿�硷紝鍒濆鍖栦竴涓敭鍊硷紝鍒嗗嚑绉嶆儏鍐碉細
//涓�绉嶆槸娌℃湁鑺傜偣锛岀洿鎺ュ拷鐣ヨ繖涓�琛岋紝
//涓�绉嶆湁鑺傜偣锛屼絾鏄兘鏄敞閲婅妭鐐癸紝涔熷拷鐣�
//鏈�鍚庝竴绉嶅氨鏄湁鐪熸鐨勮妭鐐癸紝娣诲姞閿��
//鍙傛暟index鏄娴嬭涓槸鍚﹀惈鏈夆��=鈥欒繑鍥炵殑绱㈠紩鍊�
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


//瀵规瘡涓�琛岃鍒扮殑绫诲瀷杩涜鍒嗙被锛岃妭鐐圭殑鍒濆涓鸿妭鐐瑰厓绱狅紝閿�煎垵濮嬪寲涓鸿妭鐐逛笅閿�煎厓绱狅紝
//杩斿洖鍊间负LineType
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
        //鑺傜偣
        if(strlen(p_char) > (MAX_STRING_LEN+2))
            return TOOLONG;
		
        index = initLineNode(p_inifile,p_char);
		if(index < 0)
			return index;
        return NODETYPE;
    }

    if(*line == CONTENT_COMMA_SIGN)
    {
        //娉ㄩ噴
        if(strlen(line) > MAX_STRING_LEN)
            return TOOLONG;
        initLineNotes(p_inifile,line);
        return NOTESTYPE;
    }

	index = isKeyValueLine(line);
    if(index > 0)//鍒ゆ柇鏄惁鏈�=
    {
        //閿��
        if(strlen(line) > MAX_FILLINE_LEN)
            return TOOLONG;
        index = initLineKeyValue(p_inifile,line,index);
		if(index < 0)
			return index;
        return KEYVALUE;
    }
	
    return ERRORTYPR;
 }


//璇诲彇鏂囦欢绫诲锛屼竴琛屼竴琛岀殑璇诲彇锛屼竴鐩磋鍒版枃浠剁粨鏉燂紝鎹㈣绗scii鐮佹槸10锛岀┖鏍奸敭鏄�32锛屽埗琛ㄧ鏄�9
//濡傛灉鏈変竴琛岄暱搴﹀ぇ浜庢寚瀹氶暱搴︼紝杩斿洖闀垮害閿欒锛屼細瀵艰嚧鏁翠釜绋嬪簭鍑洪敊鐨勶紝
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
			return result;//杩欎釜閿欒浼氬鑷磇ni鏂囦欢鍒濆鍖栧け璐�
		}
		memset(line,0,(MAX_FILLINE_LEN+4));
	}
	fclose(fp);
	return 0;
}

//閲婃斁ini鏂囦欢鎵�鏈夌殑鑺傜偣鍐呭瓨
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

//鏍规嵁ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡閲嶆柊鏇存柊ini鏂囦欢锛岄噸鏂板啓鏂囦欢
int updateIniFile(INI_FILE *p_inifile)
{
	int file_fd;//鎵撳紑鏂囦欢鎻忚堪绗�
	int bytes_write,buffersSize;//鍐欏瓧鑺傚ぇ灏忥紝搴旇鍐欏瓧鑺傚ぇ灏�
	char *ptr;//鎸囧悜鍐欑紦鍐插尯鐨勬寚閽�
	int errno;//閿欒鎻忚堪鍙橀噺
	IniFileNode *p = NULL;//鑺傜偣鎸囬拡
	KeyValueNode *q = NULL;//閿�兼寚閽�
	char buffer[(MAX_FILLINE_LEN+4)];//鍐欑紦鍐插尯
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
 * *  @brief       	鍒濆鍖栦竴涓猧ni鏂囦欢
 * *  @author      	zhaoxiaoxiao
 * *  @date        	2014-06-21
 * *  @fileName   	鏂囦欢璺緞
 * *  @return      	ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡锛岀┖璇存槑鍒濆鍖栧け璐ワ紝鏂囦欢涓嶅瓨鍦ㄦ垨鑰呮枃浠舵墦寮�澶辫触锛屾垨鑰呮枃浠朵腑鏌愪竴琛屽唴瀹硅秴杩囨寚瀹氶暱搴�
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
 * *  @brief       	                閫氳繃寤鸿幏鍙杋ni鏂囦欢鍊�
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡,
 * *  @parameter                    鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                鍊兼寚閽堬紱value will be lost after exitOperationIniFile
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
 * *  @brief       	                淇敼涓�涓猧ni鏂囦欢閿�硷紝鍙兘淇敼鍊硷紝涓嶈兘淇敼閿�
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @parameter                    鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                0鎿嶄綔鎴愬姛锛屽叾浠栧け璐ワ紝-1鍙傛暟閿欒锛�-2鑺傜偣娌℃湁鎵惧埌锛�-3閿�兼病鏈夋壘鍒�
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
 * *  @brief       	                澧炲姞涓�涓猧ni鏂囦欢閿��
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @parameter                    鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                0鎿嶄綔鎴愬姛锛�-1娌℃湁鍙傛暟閿欒锛�-2鑺傜偣娌℃湁鎵惧埌锛�-3鏄凡瀛樺湪閿��
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
 * *  @brief       	                鍒犻櫎涓�涓猧ni鏂囦欢閿��
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @parameter					鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                0鎿嶄綔鎴愬姛锛�-1娌℃湁鍙傛暟閿欒锛�-2鑺傜偣娌℃湁鎵惧埌锛�-3鏄病鏈夋壘鍒伴敭鍊�
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
 * *  @brief       	                鍒犻櫎涓�涓猧ni鏂囦欢鑺傜偣
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @parameter					鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                0鎿嶄綔鎴愬姛锛�-1娌℃湁鍙傛暟閿欒锛�-2鑺傜偣娌℃湁鎵惧埌锛�
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
 * *  @brief       	                澧炲姞涓�涓猧ni鏂囦欢鑺傜偣
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @parameter                    鑺傜偣鍚嶇О,鍙傛暟缁撴瀯浣擄紝璇﹁缁撴瀯浣撴弿杩�
 * *  @return      	                0鎿嶄綔鎴愬姛锛�-1娌℃湁鍙傛暟閿欒锛�-2鑺傜偣宸茬粡瀛樺湪
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
 * *  @brief       	                娓呴櫎涓�涓猧ni鏂囦欢鎵�鏈夋搷浣滐紝閲婃斁鎵�鏈夊唴瀛�
 * *  @author      	                zhaoxiaoxiao
 * *  @date        	                2014-06-21
 * *  @p_inifile     				ini鏂囦欢鍦ㄥ唴瀛樹腑鎿嶄綔鐨勯摼琛ㄥご鎸囬拡
 * *  @return      	                0鎿嶄綔鎴愬姛锛�-1鍙傛暟閿欒
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

