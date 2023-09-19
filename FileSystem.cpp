#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#define File string
using namespace std;

//函数声明
void User_Register();		//用户注册
int User_Login();			//用户登录
int User_Logout();			//用户登出
int File_Create(File name);	//创建文件
void mkdir(File name);	//创建目录
int Open_File(File name);		//打开文件
int Close_File(File name);		//关闭文件
int Read_File(File name);		//读取文件
int Delete_File(File name);		//删除文件
void Remove(File name);	//删除目录
void cd();			//切换目录
void dir();			//列文件目录
void ls();			//显示当前目录的文件
int Write_File(File name, char *, int);	//写入文件
void Change_Attribute(File name);	//修改文件属性
void User_Interaction(); 	//用户交互
void Show_All();			//列出所有命令以及用法
bool User_Login_or_not();		//检测是否有用户登录


//-------第一级：顶层目录（所有的用户）
struct Main_File_Directory   			//用户
{
	string user;	//用户名
	string pwd;	//登录密码
	struct USER_User_File_Directory *next;  //指向用户目录
};


//-------第三层：用户的某个目录文件下的所有文件（包含一个用户的所有文件）
struct User_File_Directory  //用户目录
{
	struct file_message  //每个文件夹下可以有64个文件
	{
		File filename;	//文件名
		int protect_code; 	//保护码
		int length; 		//文件长度
		int addr; 		//文件地址
	}ufd[64];
	File Directory_Name; 		//目录名
	int current_file_size = 0;		//目录下文件数量
};


//-------第二级：单个用户的文件目录
struct USER_User_File_Directory
{
	struct User_File_Directory Directory[16]; 		//用户的目录
	int user_Directory_size = 0; 	//当前用户的目录数
};


// user Open_File file：当前打开的文件控制块
struct User_Open_File  //假设一个用户最多同时打开16个文件
{
	struct uof
	{
		File filename;	//文件名
		int pointer; 		//文件的读写指针
		int protect_code; 	//文件保护码（2表示可读可写,1表示可读不可写,0表示不可读不可写）
		int addr; 		//文件地址
	}uof[16];
	int current_Open_Filefilesize = 0;	//打开的文件数
};

//-------记录文件占用磁盘块情况：物理块，假设一个磁盘的每个物理块大小为512个字节=64*2*4字节
struct File_Allocation_Table  //文件分配表   用一块物理块存放，那么最多可以记录64块数据块的信息。
{
	int next = -1; 	//下一个磁盘块号
	int used = 0; 	//1表示被使用，0表示未被使用
}fat[64];


int max_usersize = 16;		//最大用户数量
int max_userfilesize = 64;	//每个用户最大文件夹数量
int max_Open_Filefilesize = 16;	//用户可以同时打开的文件数量


Main_File_Directory mfd[16]; 					//16个用户（身份登录信息）
USER_User_File_Directory user_all_Directory_array[16];  	//16个用户的所有目录的对象（文件目录信息）
Main_File_Directory  user; 					//当前用户，可检索到当前用户下所有的目录（同时只能有一个用户处于登录状态）
User_Open_File Open_Filefile[16]; 				//当前用户的文件打开表对象，为全局变量
User_Open_File *current_Open_Filetable; 				//指向当前文件打开表


char *fdisk; 		//虚拟磁盘的起始位置
int user_size = 0; 	//记录当前用户的人数（上限16）
string File_Path; 		//记录当前用户的路径
bool User_Login_or = false; 	//记录当前是否有用户登录


//文件创建
int File_Create(File name)
{
	//判断当前路径
	if(File_Path == "")
	{
		cout << "当前不处于文件目录下，请在文件夹下创建文件" << endl;
		return -1;
	}

	//获取文件夹下标
	int index; //标识当前目录在Directory数组中第几个
	for (index = 0; index < user.next->user_Directory_size; index++)	//遍历当前用户所有的文件夹
	{
		if (File_Path == user.next->Directory[index].Directory_Name)	//判断
			break;
	}

	//文件重名判断
	int i;
	for (i = 0; i < user.next->Directory[index].current_file_size; i++)   //遍历当前目录，查看是否有文件重名
	{
		if (name == user.next->Directory[index].ufd[i].filename)
			break;
	}
	if (i < user.next->Directory[index].current_file_size) //判断文件名是否重复
	{
		cout << "文件名重复" << endl;
		return -1;
	}

	//文件数目判断
	if (user.next->Directory[index].current_file_size == 64)  //判断当前目录的文件到达64个
	{
		cout << "用户文件已经达到64个" << endl;
		return -1;
	}

	//文件是否可分配到新的空闲块
	//寻找空闲块
	int j;
	for (j = 0; j < 64; j++)  //判断是否有空的空闲块。
	{
		if (fat[j].used == 0)
			break;
	}
	if (j >= 64)
	{
		cout << "磁盘没有空闲块了" << endl;
		return -1;
	}

	//创建文件：修改ufd信息
	user.next->Directory[index].ufd[user.next->Directory[index].current_file_size].filename = name;	//文件命名
	user.next->Directory[index].ufd[user.next->Directory[index].current_file_size].addr = j; 		//文件起始盘块号
	user.next->Directory[index].ufd[user.next->Directory[index].current_file_size].length = 0; 	//文件初始没有数据
	user.next->Directory[index].ufd[user.next->Directory[index].current_file_size].protect_code = 2; 	//默认可读可写
	user.next->Directory[index].current_file_size++;							//用户文件数量加1
	fat[j].used = 1; 	//磁盘块被使用
	fat[j].next = -1; 	//只是个空文件,所有没有后序的块
	cout << "文件创建成功" << endl;
}

//打开文件
int Open_File(File name)
{
	//遍历该用户所有的文件夹
	int index; //标识当前目录在Directory数组中第几个
	for (index = 0; index < user.next->user_Directory_size; index++)
	{
		if (File_Path == user.next->Directory[index].Directory_Name)	//找指定名称的目录
		{
			break;
		}
	}

	//遍历该文件夹下的文件
	int i;
	for (i = 0; i < user.next->Directory[index].current_file_size; i++) //遍历目录下的文件
	{
		if (name == user.next->Directory[index].ufd[i].filename)
			break;
	}

	//判断文件是否存在
	if (i >= user.next->Directory[index].current_file_size)
	{
		cout << "该用户没有这个文件" << endl;
		return -1;
	}

	//判断文件是否可以被打开（是否达到打开上限）
	if (current_Open_Filetable->current_Open_Filefilesize == max_Open_Filefilesize) //如果打开文件的数量达到最大值，那么就无法打开
	{
		cout << "文件打开数量已经达到最大值" << endl;
		return -1;
	}

	//判断文件是否已经被打开
	for (int j = 0; j < current_Open_Filetable->current_Open_Filefilesize; j++) //遍历打开文件
	{
		if (current_Open_Filetable->uof[j].filename == name)
		{
			cout << "文件已经打开" << endl;
			return -1;
		}
	}

	int k;
	for (k = 0; k < user.next->Directory[index].current_file_size; k++) //找到要打开的文件在文件数组中的第几个
	{
		if (user.next->Directory[index].ufd[k].filename == name)
			break;
	}

	//打开文件：更新打开表（保存被打开文件的信息），
	current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize].filename = name;
	current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize].protect_code = user.next->Directory[index].ufd[k].protect_code;
	current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize].pointer = user.next->Directory[index].ufd[k].length;
	current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize].addr = user.next->Directory[index].ufd[k].addr;
	current_Open_Filetable->current_Open_Filefilesize++; //文件打开数量加1
	cout << "文件打开成功" << endl;
	return k; //返回文件在文件打开表中的第几项
}


//修改文件属性
void Change_Attribute(File name)
{
	int index; //标识当前目录在Directory数组中第几个
	for (index = 0; index < user.next->user_Directory_size; index++)	//遍历该用户所有的文件夹
	{
		if (File_Path == user.next->Directory[index].Directory_Name)		//找指定名称的目录
			break;
	}

	int i;
	for (i = 0; i < user.next->Directory[index].current_file_size; i++)	//遍历该文件夹下的文件
	{
		if (name == user.next->Directory[index].ufd[i].filename)
			break;
	}

	//判断文件是否存在
	if (i >= user.next->Directory[index].current_file_size)
	{
		cout << "该用户没有这个文件" << endl;
		return ;
	}

	int j;
	for (j = 0; j < current_Open_Filetable->current_Open_Filefilesize; j++)  //判断该文件是否被打开
	{
		if (current_Open_Filetable->uof[j].filename == name)
			break;
	}
	if (j < current_Open_Filetable->current_Open_Filefilesize) //说明文件被打开了
	{
		cout << "该文件已被打开，无法修改" << endl;
		return;
	}

	//修改文件属性：修改ufd信息
	while(1)
	{
		cout << "请输入需要修改的属性对应的数字（1--文件名）（2--文件读写保护码）：";
		int Chosen_Attribute;
		cin >> Chosen_Attribute;	//选项
		if(Chosen_Attribute == 1)
		{
			cout << "请输入新的文件名：";
			File New_File_Name;
			cin >> New_File_Name;	//文件名


			//文件重名判断
			int k,l;
			for (k = 0; k < user.next->Directory[index].current_file_size; k++) //找到要打开的文件在文件数组中的第几个
			{
				if (user.next->Directory[index].ufd[k].filename == name)
					break;
			}

			for (l = 0; l < user.next->Directory[index].current_file_size; l++)   //遍历当前目录，查看是否有文件重名
			{
				if (New_File_Name == user.next->Directory[index].ufd[l].filename && l!=k)
					break;
			}
			if (l < user.next->Directory[index].current_file_size) //判断文件名是否重复
			{
				cout << "文件名重复" << endl;
				return;
			}

			user.next->Directory[index].ufd[i].filename = New_File_Name;
			cout << "文件名修改成功！" << endl;
			break;
		}
		else if(Chosen_Attribute == 2)
		{
			while(1)
			{
				cout << "请输入文件的读写权限对应的数字（0--禁止读写）（1--仅可读）（2--可读可写）：";
				int code;
				cin >> code;	//读写权限
				if(code == 0 || code == 1 || code == 2 )
				{
					user.next->Directory[index].ufd[i].protect_code = code;
					cout << "文件属性修改成功！" << endl;
					break;
				}
				else
					cout << "输入有误，请重新输入" << endl;
			}
			break;
		}
		else
		{
			cout << "输入有误，请重新输入" << endl;
		}
	}
}

//关闭文件
int Close_File(File name)
{
	int fd;
	for (int i = 0; i < current_Open_Filetable->current_Open_Filefilesize; i++)  //找到要关闭的文件在表中的第几项
	{
		if (current_Open_Filetable->uof[i].filename == name)	//根据文件名查找
		{
			fd = i;
			break;
		}
	}

	if (fd >= current_Open_Filetable->current_Open_Filefilesize)
	{
		cout << "没有这个文件或者文件没有打开" << endl;
		return -1;
	}

	//将要删除的项目与最后一个项目交换,再将current_Open_Filefilesize--就等价于删除）
	current_Open_Filetable->uof[fd].filename = current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize - 1].filename;
	current_Open_Filetable->uof[fd].pointer = current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize - 1].pointer;
	current_Open_Filetable->uof[fd].protect_code = current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize - 1].protect_code;
	current_Open_Filetable->uof[fd].addr = current_Open_Filetable->uof[current_Open_Filetable->current_Open_Filefilesize - 1].addr;
	current_Open_Filetable->current_Open_Filefilesize--;
	cout << "文件关闭成功" << endl;
	return 0;
}

//删除文件
int Delete_File(File name)
{
	//找到当前目录
	int index; //标识当前目录在Directory数组中第几个
	for (index = 0; index < user.next->user_Directory_size; index++)
	{
		if (File_Path == user.next->Directory[index].Directory_Name)
			break;
	}

	//遍历当前目录下的文件
	int i;
	for (i = 0; i < user.next->Directory[index].current_file_size; i++)  //判断当前目录下有没有这个文件
	{
		if (user.next->Directory[index].ufd[i].filename == name)
			break;
	}
	if (i >= user.next->Directory[index].current_file_size)
	{
		cout << "没有这个文件" << endl;
		return -1;
	}

	//遍历打开的文件
	int j;
	for (j = 0; j < current_Open_Filetable->current_Open_Filefilesize; j++)  //判断该文件是否被打开
	{
		if (current_Open_Filetable->uof[j].filename == name)
			break;
	}
	if (j < current_Open_Filetable->current_Open_Filefilesize) //说明文件被打开了
	{
		cout << "该文件已被打开，无法删除" << endl;
		return -1;
	}

	//更新当前用户目录下文件数组信息,就是将最后一个文件的信息替换到要删除的文件的位置
	fat[user.next->Directory[index].ufd[i].addr].used = 0; //没有使用
	user.next->Directory[index].ufd[i].filename = user.next->Directory[index].ufd[user.next->Directory[index].current_file_size - 1].filename;
	user.next->Directory[index].ufd[i].addr = user.next->Directory[index].ufd[user.next->Directory[index].current_file_size - 1].addr;
	user.next->Directory[index].ufd[i].length = user.next->Directory[index].ufd[user.next->Directory[index].current_file_size - 1].length;
	user.next->Directory[index].ufd[i].protect_code = user.next->Directory[index].ufd[user.next->Directory[index].current_file_size - 1].protect_code;
	user.next->Directory[index].current_file_size--; //用户文件数量减1

	//回收磁盘：更新文件分配表fat
	int temp = fat[user.next->Directory[index].ufd[i].addr].next;
	while (temp != -1)
	{
		fat[temp].used = 0;
		temp = fat[temp].next;
	}
	cout << "删除文件成功" << endl;
	return 0;
}

//读取文件
int Read_File(File name)
{
	//文件目录下标
	int index1; //标识当前目录在Directory数组中第几个
	for (index1 = 0; index1 < user.next->user_Directory_size; index1++)
	{
		if (File_Path == user.next->Directory[index1].Directory_Name)
		{
			break;
		}
	}

	//文件下标
	int a; //遍历文件
	for (a = 0; a < user.next->Directory[index1].current_file_size; a++)    //判断文件是否存在
	{
		if (user.next->Directory[index1].ufd[a].filename == name)
			break;
	}
	if (a >= user.next->Directory[index1].current_file_size)
	{
		cout << "没有这个文件" << endl;
		return -1;
	}

	int i;
	//判读文件是否打开
	for (i = 0; i < current_Open_Filetable->current_Open_Filefilesize; i++)
	{
		if (current_Open_Filetable->uof[i].filename == name)
			break;
	}
	if (i >= current_Open_Filetable->current_Open_Filefilesize)
	{
		cout << "文件没有打开，无法读取" << endl;
		return -1;
	}

	//如果文件已经打开，那么此时的 i 就是打开的文件数组uof的下标
	int fd = i; //获取文件描述字

	//判断读文件的合法性
	if (current_Open_Filetable->uof[fd].protect_code == 0)
	{
		cout << "文件不可读" << endl;
		return -1;
	}
	else
	{
		int len = current_Open_Filetable->uof[fd].pointer; //文件的长度
		int block_size = len / 512; //磁盘的个数
		int offset = len % 512; //偏移量
		if (offset != 0)
			block_size++;	//包含偏移量的磁盘
		//如果用一个文件表示磁盘的引导块，用另一个文件表示磁盘的数据块，那么我们计算文件的起始位置就不用加上磁盘的引导块了
		//关于文件的存放文件，我们char *fdisk表示一整个磁盘，然后不同文件的内容存放在这个指针所指向的不同字符段
		char * first = fdisk + current_Open_Filetable->uof[fd].addr * 512; //文件的起始地址
		char * buf = (char *)malloc(512 * sizeof(char)); //缓冲区（大小等于一个空闲块大小）

		cout << "文件的内容为：";
		for (int k = 0; k < block_size; k++)	//遍历文件包含的块，k代表相对起址的块号
		{
			if (k == block_size - 1)  //如果是最后一个磁盘块，就不是将全部512字节输出，而是输出偏移量大小的数据
			{
				for (int j = 0; j < len - k * 512; j++)	//赋值文件剩余的字符------偏移量
				{
					buf[j + k * 512] = first[j];	//缓冲区存放待输出的字符
				}
				for (int u = 0; u < len - k * 512; u++)
				{
					cout << buf[u + k * 512];	//输出剩余长度，之所以这样输出，printf()，将整个buf的内容全部输出，如果没有读满就会出现乱码
				}
			}
			else //不在最后一个磁盘块，也就是在其他已经读满的磁盘块
			{
				for (int j = 0; j < len - i * 512; j++)
					buf[j + k * 512] = first[j];	//缓冲区读满就输出内容
				printf("%s\n", buf);	//输出文件的内容
				int next_block = fat[current_Open_Filetable->uof[fd].addr].next;	//读完一个磁盘块后，再接着读下一个磁盘块
				first = fdisk + next_block * 512;
			}
		}
		cout << endl;
		cout << "文件读取成功" << endl;
		free(buf); //释放缓冲区
		return 0;
	}

}

//文件写入
int Write_File(File name, char * buf, int len)
{
	//获取文件目录数组下标（当前是哪个文件夹）
	int index1; //标识当前目录在Directory数组中第几个
	for (index1 = 0; index1 < user.next->user_Directory_size; index1++)
	{
		if (File_Path == user.next->Directory[index1].Directory_Name)
			break;
	}

	int i;
	for (i = 0; i < current_Open_Filetable->current_Open_Filefilesize; i++)
	{
		if (current_Open_Filetable->uof[i].filename == name)
			break;
	}

	int fd = i;

	int temp; //保存当前所写的文件在用户文件目录表的第几项，为了后面修改文件的大小
	int first_block = current_Open_Filetable->uof[fd].addr; //用户文件存放的第一个磁盘块
	//遍历当前目录下所有文件，获取文件下标temp（打开表下标是fd ； 文件下标是temp）
	for (int k = 0; k < user.next->Directory[index1].current_file_size; k++)
	{
		if (user.next->Directory[index1].ufd[k].addr == first_block)
		{
			temp = k;
			break;
		}
	}

	//追加写
	//找到该文件存放的最后一个磁盘块
	while (fat[first_block].next != -1)
	{
		first_block = fat[first_block].next;
	}

	//计算该文件存放的最后一个地址
	char  * first;
	first = fdisk + first_block * 512 + current_Open_Filetable->uof[fd].pointer % 512;


	if (len <= 512 - current_Open_Filetable->uof[fd].pointer % 512)	//如果最后一个文件剩下的空间大于要写入的长度
	{
		for (int i = 0; i < len; i++)
		{
			first[i] = buf[i];//将缓冲区的内容写入虚拟磁盘中
		}
		current_Open_Filetable->uof[fd].pointer = current_Open_Filetable->uof[fd].pointer + len;  //更新文件打开表
		user.next->Directory[index1].ufd[temp].length = user.next->Directory[index1].ufd[temp].length + len; //更新用户目录文件表
	}
	else	//如果磁盘剩下的空间不足写入
	{
		//写入一部分的内容到最后一个磁盘块的剩余空间
		for (i = 0; i < 512 - current_Open_Filetable->uof[fd].pointer % 512; i++)
			first[i] = buf[i];
		//计算分配完最后一个磁盘的剩余空间后，还剩下多少字节没有存储，计算还需要分配多少空闲块
		int last_size = len - (512 - current_Open_Filetable->uof[fd].pointer % 512); //剩余待写入的大小
		int need_block_size = last_size / 512;	//待分配的空闲块数
		int need_offset_size = last_size % 512;	//偏移量
		if (need_offset_size > 0)
			need_block_size++; //总共需要这么磁盘块

		//判断磁盘剩余空间是否足够
		int unused_block_size = 0; //记录没有使用过的磁盘块的个数
		for (int i = 0; i < 64; i++)
		{
			if (fat[i].used == 0)
				unused_block_size++;
		}

		if (unused_block_size < need_block_size)	//磁盘剩余空间不足
		{
			cout << "磁盘没有空间存放了" << endl;
			return -1;
		}
		else	//磁盘还有足够的空间
		{
			int item = current_Open_Filetable->uof[fd].addr;
			for (int p = 0; p < need_block_size; p++) //执行多次寻找空闲磁盘的操作，
			{
				for (int i = 0; i < 64; i++)
				{
					if (fat[i].used == 0) //没有被使用
					{
						first = fdisk + i * 512;	//当前要写入的磁盘块的起始地址
						fat[i].used = 1;		//标记被使用
						fat[item].next = i;		//标记下一个磁盘
						item = i;
						break;
					}
				}
				if (p == need_block_size - 1)
				{
					for (int k = 0; k < need_offset_size; k++)  //将文件的偏移量写入最后一个文件中
						first[k] = buf[k];
					fat[i].next = -1;
				}
				else  //如果不是最后一个空闲块
				{
					for (int k = 0; k < 512; k++)
						first[k] = buf[k];
				}
			}
			//更新文件打开表
			current_Open_Filetable->uof[fd].pointer = current_Open_Filetable->uof[fd].pointer + last_size;
			//更新用户目录文件表
			user.next->Directory[index1].ufd[temp].length = user.next->Directory[index1].ufd[temp].length + last_size;
		}
	}
	cout << "文件写入成功" << endl;
	return 0;
}

// 列文件目录
void dir()
{
	int index1; //标识当前目录在Directory数组中第几个
	for (index1 = 0; index1 < user.next->user_Directory_size; index1++)
	{
		if (File_Path == user.next->Directory[index1].Directory_Name)
			break;
	}
	if (File_Path == "") //表示此时路径在用户的目录表，显示文件目录
	{
		cout << "\t" << "目录名" << endl;
		for (int i = 0; i < user.next->user_Directory_size; i++)
			cout << "\t" << user.next->Directory[i].Directory_Name << endl;
	}
	else  //显示目录下的文件
	{
		cout << "\t" << "文件名" << "\t" << "文件起始盘块号" << "\t" << "文件保护码" << "\t" <<"文件长度" << endl;
		for (int i = 0; i < user.next->Directory[index1].current_file_size; i++)  //输出文件的信息
		{
			cout << "\t" << user.next->Directory[index1].ufd[i].filename << "\t" << user.next->Directory[index1].ufd[i].addr << "\t" << "\t" << user.next->Directory[index1].ufd[i].protect_code <<"\t" <<"\t" << user.next->Directory[index1].ufd[i].length << endl;
		}
	}
}

//登录
int User_Login()
{
	string name;	//用户名
	string pwd;//密码
	cout << "请输入用户名：" << endl;	//用户输入
	cin >> name;
	cout << "请输入密码：" << endl;
	cin >> pwd;
	int i; // 用户目录循环变量
	for ( i = 0; i < user_size; i++)	//遍历用户目录mfd
	{
		if (mfd[i].user == name)
		{
			if (mfd[i].pwd == pwd)
				break;
			else
			{
				cout << "密码错误" << endl;
				break;
			}
		}
	}
	//如果遍历一遍之后，没有任何一项匹配成功，给出提示信息并返回
	if (i >= user_size)
	{
		cout << "没有这个用户" << endl;
		return -1;
	}

	//信息验证成功，分配内存
	mfd[i].next = & (user_all_Directory_array[i]); //用户指向自己的所有目录的结构

	//初始化当前用户的信息
	user = mfd[i];
    	user.next->user_Directory_size = mfd[i].next->user_Directory_size; //当前用户的文件夹数量

	user_size++; //用户人数++
	current_Open_Filetable = &Open_Filefile[user_size]; //指针指向文件打开表对象
	current_Open_Filetable->current_Open_Filefilesize = 0; //设初始值（初始打开的文件数为0）
	File_Path = ""; //指定当前路径为用户的全部目录处
	User_Login_or = true; //当前有用户登录
	return 1;
}

int User_Logout()
{
	user.user = "";
	File_Path = "";
	User_Login_or=false;
	return 1;
}

void cd()
{
	string temp_File_Path;
	cin >> temp_File_Path;
	if (temp_File_Path == "..")	//返回到根目录
	{
		File_Path = "";
		return;
	}

	int i;	//遍历文件目录
	for (i = 0; i < user.next->user_Directory_size; i++)  //判断路径是否存在
	{
		if (temp_File_Path == user.next->Directory[i].Directory_Name)
			break;
	}
	if (i >= user.next->user_Directory_size)
	{
		cout << "没有这个目录" << endl;
		return;
	}
	File_Path = temp_File_Path;
	return;
}

// 创建目录
void mkdir(string name)
{
	//判断当前用户的文件目录的数目是否达到最大值
	if (user.next->user_Directory_size == 16)
	{
		cout << "用户目录已经达到最大值，不能再创建目录了" << endl;
		return;
	}

	//遍历目录
	int i;
	for (i = 0; i < user.next->user_Directory_size; i++)  //判断创建的目录是否存在
	{
		if (user.next->Directory[i].Directory_Name == name)
			break;
	}
	if (i < user.next->user_Directory_size) //找到同名后代表已经存在
	{
		cout << "该目录已经存在了" << endl;
		return;
	}

	//如果文件夹可以创建，最后一个下标位置下，创建新的目录
	user.next->Directory[user.next->user_Directory_size].Directory_Name = name;	//目录名
	user.next->Directory[user.next->user_Directory_size].current_file_size = 0; 	//新创建的目录里面的文件个数为0
	user.next->user_Directory_size++; //用户的目录数加1

	cout << "创建目录成功" << endl;
	return;
}

// 检测登录状态
bool User_Login_or_not()
{
	if(!User_Login_or)
		return 0;
	else
		return 1;

}

// 显示用户创建的文件
void ls()
{
	if(File_Path == "")	//如果是根目录
	{
		for (int i = 0; i < user.next->user_Directory_size; i++)  //遍历创建的目录
		{
			cout << user.next->Directory[i].Directory_Name << "\t" ;
		}
	}
	int index1; //标识当前目录在Directory数组中的下标
	for (index1 = 0; index1 < user.next->user_Directory_size; index1++)
	{
		if (File_Path == user.next->Directory[index1].Directory_Name)
			break;
	}
	for (int a = 0; a < user.next->Directory[index1].current_file_size; a++)    //遍历文件
	{
		cout << user.next->Directory[index1].ufd[a].filename << "\t" ;
	}
	cout << endl;
}

void User_Interaction()  //用户输入命令
{
	if (user.user == "")
		cout << "localhost :";
	else
		cout << user.user << "@localhost  home/" <<File_Path << ">";

	string operation;
	cin >> operation;

	if (operation == "login")
	{
		User_Login();
	}
	else if(operation !="login" && operation !="register" && operation !="help" && operation !="clear" && operation !="exit" && !User_Login_or_not())
	{
		cout<<"当前没有登录，请使用User_Login进行登录，或者使用register进行注册"<<endl;
	}
	else if (operation=="logout" && User_Login_or_not())
	{
		User_Logout();
	}
	else if (operation == "dir" && User_Login_or_not())
	{
		dir();
	}
	else if (operation == "create" && User_Login_or_not())
	{
		File filename;
		cin >> filename;
		File_Create(filename);
	}
	else if (operation == "delete" && User_Login_or_not())
	{
		File filename;
		cin >> filename;
		Delete_File(filename);
	}
	else if (operation == "open" && User_Login_or_not())
	{
		File name;
		cin >> name;
		Open_File(name);
	}
	else if (operation == "close" && User_Login_or_not())
	{
		File name;
		cin >> name;
		Close_File(name);
	}
	else if (operation == "read" && User_Login_or_not())
	{
		File name;
		cin >> name;
		Read_File(name);
	}
	else if (operation == "write" && User_Login_or_not())
	{
		File content;
		File name;
		cin >> name;

		int i;
		//判读文件是否打开
		for (i = 0; i < current_Open_Filetable->current_Open_Filefilesize; i++)
		{
			if (current_Open_Filetable->uof[i].filename == name)
				break;
		}
		if (i >= current_Open_Filetable->current_Open_Filefilesize)
		{
			cout << "文件没有打开，无法写入" << endl;
			return;
		}

		int fd = i; //获取文件描述字
		//判断读文件的合法性
		if (current_Open_Filetable->uof[fd].protect_code != 2)
		{
			cout << "文件不可写" << endl;
			return;
		}

		cin.ignore(); //清空缓冲区的内容
		cout << "请输入文件要写入的内容: " << endl;;
		getline(cin, content); //读入一整行内容
		char buf[512];

		int times = content.length() / 512;
		int offset = content.length() % 512;
		if (offset != 0)
			times++;
		for (int i = 0; i < times; i++)
		{
			if (i == times - 1) //注意这里不能写成times--
			{
				for (int j = 0; j < offset; j++)
					buf[j] = content[j];
			}
			else
			{
				for (int j = 0; j < 512; j++)
					buf[j] = content[j];
			}
			Write_File(name, buf, content.length());
		}
	}
	else if (operation == "ls" && User_Login_or_not())
	{
		ls();
	}
	else if (operation == "exit")
	{
		exit(0);
	}
	else if (operation == "cd" && User_Login_or_not())
	{
		cd();
	}
	else if (operation == "mkdir" && User_Login_or_not())
	{
		File name;
		cin >> name;
		mkdir(name);
	}
	else if (operation == "register")
	{
		User_Register();
	}
	else if (operation == "remove" && User_Login_or_not())
	{
		File name;
		cin >> name;
		Remove(name);
	}
	else if (operation == "change" && User_Login_or_not())
	{
		File name;
		cin >> name;
		Change_Attribute(name);
	}
	else if (operation == "help")
	{
		Show_All();
	}
	else if (operation == "clear")
	{
		system("clear");
	}
	else
	{
		cout << "命令错误，请重新输入" << endl;
	}
}

void User_Register()  //用户注册
{
	cout << "请输入用户名：";
	string user;	//用户名
	cin >> user;
	cout << "请输入密码：";
	string pwd;	//密码
	cin >> pwd;
	int i;
	for (i = 0; i < 16; i++)	//判断用户名是否存在
	{
		if (mfd[i].user == user)	//如果已经存在
		{
			cout << "该用户已经存在" << endl;
			return;
		}
	}
	mfd[user_size].user = user;	//保存在mfd中（第一级目录）
	mfd[user_size].pwd = pwd;
	user_size++; //用户人数加1

	cout << "用户注册成功！" << endl;
}

void Remove(string name)  //删除目录
{
	int index;
	int i;
	for (i = 0; i < user.next->user_Directory_size; i++)
	{
		if (name == user.next->Directory[i].Directory_Name)
		{
			index = i;
			break;
		}
	}
	if (i >= user.next->user_Directory_size)
	{
		cout << "目录不存在" << endl;
		return;
	}

	for (int i = 0; i < user.next->Directory[index].current_file_size; i++)   //删除目录里面的文件
	{//直接释这些文件所占的磁盘块
		fat[user.next->Directory[index].ufd[i].addr].used = 0; //没有使用
		int temp = fat[user.next->Directory[index].ufd[i].addr].next;
		while (temp != -1)
		{
			fat[temp].used = 0;
			temp = fat[temp].next;
		}
	}
	//删除目录项，就是将两个目录项的内容进行交换

	user.next->Directory[index].current_file_size = user.next->Directory[user.next->user_Directory_size-1].current_file_size;  //注意这里需要减一，由于本身结构的限制
	user.next->Directory[index].Directory_Name = user.next->Directory[user.next->user_Directory_size-1].Directory_Name;
	for (int i = 0; i < user.next->Directory[user.next->user_Directory_size-1].current_file_size; i++)  //注意这里的减一
	{
		user.next->Directory[index].ufd[i].addr = user.next->Directory[user.next->user_Directory_size-1].ufd[i].addr;
		user.next->Directory[index].ufd[i].filename = user.next->Directory[user.next->user_Directory_size-1].ufd[i].filename;
		user.next->Directory[index].ufd[i].length = user.next->Directory[user.next->user_Directory_size-1].ufd[i].length;
		user.next->Directory[index].ufd[i].protect_code = user.next->Directory[user.next->user_Directory_size-1].ufd[i].protect_code;
	}
	user.next->user_Directory_size--; //目录数量减1
	cout << "删除目录成功" << endl;
	return;
}

void Show_All()  //展示命令
 {

	string menu = "";
	menu += "+———————————————————————————————————————————————————————————————————————————————+\n";
	menu +="|\t\t\t\t文件系统命令一览\t\t\t\t|\n";
	menu += "|———————————————————————————————————————————————————————————————————————————————|\n";
	menu += "|\t命令\t\t功能\t\t|\t命令\t\t功能\t\t|\n";
	menu += "|———————————————————————————————————————————————————————————————————————————————|\n";
	menu += "|\tregister\t注册\t\t|\thelp\t\t显示命令\t|\n";
	menu += "|\tlogin\t\t登录\t\t|\tclear\t\t清屏\t\t|\n";
	menu += "|\tlogout\t\t退出登录\t|\texit\t\t退出系统\t|\n";
	menu += "|———————————————————————————————————————————————————————————————————————————————|\n";
   	menu += "|\t\t目录操作\t\t|\t\t文件操作\t\t|\n";
	menu += "|\tmkdir (name)\t创建目录\t|\tcreate (name)\t创建文件\t|\n";
	menu += "|\tremove (name)\t删除目录\t|\tdelete (name)\t删除文件\t|\n";
	menu += "|\tcd\t\t切换目录\t|\topen (name)\t打开文件\t|\n";
	menu += "|\tdir\t\t列出目录\t|\tclose (name)\t关闭文件\t|\n";
	menu += "|\tls\t\t列出文件信息\t|\tread (name)\t读取文件\t|\n";
	menu += "|\t\t\t\t\t|\twrite (name)\t写入文件\t|\n";
	menu += "|\t\t\t\t\t|\tchange (name)\t更改文件属性\t|\n";
	menu += "+———————————————————————————————————————————————————————————————————————————————+\n";
	cout<<menu;

}
void Read_File_user(){
	string user[3]={"qzr","zty","bjw"};	//用户名
	string pwd[3]={"123","123","123"};	//密码
	for( int i=0;i<3;i++){
	mfd[user_size].user = user[i];	//保存在mfd中（第一级目录）
	mfd[user_size].pwd = pwd[i];
	user_size++; //用户人数加1
	}
}
int main()
{
	user.user = ""; //初始化当前用户的用户名为空
	File_Path = ""; //文件路径
	fdisk = (char *)malloc(1024 * 1024 * sizeof(char)); //用内存模拟外存，申请内存空间,初始化
	Read_File_user();
	Show_All(); 
	while (true)
		User_Interaction();

	free(fdisk); //程序结束，释放资源 
	return 0;
}

