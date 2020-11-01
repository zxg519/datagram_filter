/**
   by Dr. Xiaoguo Zhang, Southeast Univ.
   xgzhang at seu.edu.cn
   
*/

#ifndef ___IHAFPIAUW98ERQFIJAFDWERWERJWQRWQERASDFOASFASDFSADFSADFSADFASDFASFDASFSADFSADFSADFEWRWEFWQDSAGIMJJIOISDFASF_DATAGRAMFILTER_H____
#define ___IHAFPIAUW98ERQFIJAFDWERWERJWQRWQERASDFOASFASDFSADFSADFSADFASDFASFDASFSADFSADFSADFEWRWEFWQDSAGIMJJIOISDFASF_DATAGRAMFILTER_H____


#include <vector>
#include <iostream>
using namespace std;


//=========================================================================================================
// 用户定制的真正有效报文的处理类，用户需要自己派生这个类
//
//  当数据过滤器实现了一组有效报文的读取并去除报文头和报文尾后，会触发本类的 OnValidMessage(...)函数
//=========================================================================================================
class CDatagramFilterListener
{
public:
	// API 1
	// 截获了需要的报文，并且把报文头尾给干掉，留下“纯净”的报文
	virtual void OnValidMessage(const int msg_type_id,          // 报文组号，这个是用户自己指定的，如果用户只有一组类型报文不需要制定，可以用任意int值
		const char* msg,                // 有效报文存储位置，注意可能二进制数据，不能想当然认为是字符串
		const int msg_size              // 有效报文长度！
		) = 0;  // msg_type_id 报文类型，这个可以有用户指定
public:
	// 报告原始报文中存在的问题：如掉包，报文给串改....
	virtual void on_error_msg(const char* err_msg)
	{
		cout << err_msg << endl;
	}
};

struct binary_data
{
	const char* data;
	int         data_size;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// 报文分离类，可以实现把报文流中抽取具有特定格式的报文，并得到干净报文，并交CDatagramFilterListener侦听类处理
//    注意不得给CDatagramFilter的派生类的成员动态分配内存或者资源！！！
//注意：
//    operator<<()函数本质上调用了input()函数，也可以直接调用input()函数
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDatagramFilter
{
public:
	// currrently messagefiler is in which state ???
	enum WORK_STATE
	{
		CHECKING_HEADER,	// check signal header
		CHECKING_TAIL,      // CHECKING MESSAGE TAIL
		READING_DATA,		// reading useful signal
		READ_RESET
	};

public:
	CDatagramFilter(int msg_type_id,               // 报文组别！如果没有多组报文，可以不关心，这个参数会被传递到CDatagramFilterListener里面的回掉函数中、、
		CDatagramFilterListener* plistener = NULL    // 事件侦听器，如果匹配成功，回调CDatagramFilterListener::OnValidMessage()
		);
	virtual ~CDatagramFilter(){}

public:
	virtual void reset() = 0;
public:
	// 读入一个数据匹配，实际上就是逐个字符读入匹配！
	CDatagramFilter& operator << (const char c)
	{
		input_inner(c);
		return *this;
	}
	CDatagramFilter& operator << (const char* str)
	{
		input(str, strlen(str));
		return *this;
	}
	CDatagramFilter& operator << (const binary_data& b)
	{
		input(b.data, b.data_size);
		return *this;
	}
	void install_listener(CDatagramFilterListener* plistener = NULL)
	{
		m_plistener = plistener;
	}

public:
	// 读入一堆数据匹配
	inline WORK_STATE input(const char c)
	{
		return input_inner(c);
	}
	WORK_STATE input(const char * msg, int length = 0);

public:
	int  get_msg_type_id() 	{ return m_msg_type_id; }

protected:
	virtual WORK_STATE input_inner(const char c) = 0;

protected:
	CDatagramFilterListener * m_plistener;
	int                      m_msg_type_id;  //报文组别，这个由用户指定！！！
};
////////////////////////////////////////////////////////////////////////////////////////////////////////
// 报文分离机，使用它，可以实现对不同报文格式报文的批量处理！
//          注意：添加的报文处理器参数对应的对象必须是动态分配的！！！
// 
//      CDatagrammFilterMachine::operator<<()函数本质上调用了input()函数，也可以直接调用input()函数
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDatagrammFilterMachine
{
public:
	void add_filter(CDatagramFilter* p_filter);   // 加入报文过滤器，注意：必须是动态生成的！！！

	// 读入一个数据匹配，实际上就是逐个字符读入匹配！
	CDatagrammFilterMachine& operator << (const char c);
	CDatagrammFilterMachine& operator << (const char* str);
	CDatagrammFilterMachine& operator << (const binary_data& data)
	{
		input(data.data, data.data_size);
		return *this;
	}


public:
	// 读入一堆数据匹配
	void input(const char c);
	void input(const char * msg, const int length = 0);
	void reset();


public:
	void install_listener(CDatagramFilterListener* plistener = NULL);

public:
	~CDatagrammFilterMachine();

protected:
	vector < CDatagramFilter* > m_array;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////
//  真正干事的派生类来了
//   to be done!
///////////////////////////////////////////////////////////////////////////////////////////////////////

//==========================================================================================================================
// 具有报文头，报文尾的不定长报文的处理器
//    注意一般好的报文头和报文尾，都设计成内部无自相似性的(即KMP算法时，每次失配都直接滑动)
//         本类要求如此！
//
//   ”￥GPSGGA“  -》 good
//   "ababc"     -> bad!!!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CHeadTailDatagramFilter : public CDatagramFilter
{
public:
	CHeadTailDatagramFilter(int msg_type_id,
							const char* p_head,
							const char* p_tail,
							int head_length = 0,
							int tail_length = 0);
	virtual void reset();

protected:
	// 实现处理数据流的核心逻辑，被operator <<, input()函数调用！！！
	virtual WORK_STATE input_inner(const char c);

protected:
	vector<char>  m_header;      // 报文头串
	vector<char>  m_tail;        // 报文尾设计

	int           m_header_len;  // 冗余变量，纯粹为了加速！！！
	int           m_tail_len;    // 冗余变量，纯粹为了加速！！！

	int          m_cursor;       // 匹配报文头（或者报文尾）的当前位置

	vector<char>   m_sBuffer;  // 存储报文
	WORK_STATE     m_state;
};

//==========================================================================================================================
//   定长报文过滤器！！！
//   ”￥GPSGGA“  -》 good
//   "ababc"     -> bad!!!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CFixSizedDatagramFilter : public CDatagramFilter
{
public:
	CFixSizedDatagramFilter(int msg_type_id,
							const char* p_head,
							int head_length,
							int datagram_length,
							const char* p_tail = NULL,   // 支持核对报文尾，如果输入为0，则不支持！
							int tail_length = 0);

	virtual void reset();

protected:
	// 实现处理数据流的核心逻辑，被operator <<, input()函数调用！！！
	virtual WORK_STATE input_inner(const char c);

protected:
	vector<char>  m_header;      // 报文头串
	vector<char>  m_tail;        // 报文尾设计

	int           m_header_len;  // 冗余变量，纯粹为了加速！！！
	int           m_tail_len;    // 冗余变量，纯粹为了加速！！！

	int          m_cursor;        // 匹配报文头（或者报文尾）的当前位置
	int          m_datagram_length;
	vector<char>   m_sBuffer;  // 存储报文
	WORK_STATE     m_state;
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// two sample listeners for reference
class CTxtDatagramFilterListener : public CDatagramFilterListener
{
public:
	virtual void OnValidMessage(const int msg_type_id,
		const char* msg,
		const int msg_size)
	{
		cout << "Received msg type:" << msg_type_id << endl << "    ";
		cout << msg << endl;
	}
};
class CBinDatagramFilterListener : public CDatagramFilterListener
{
public:
	virtual void OnValidMessage(const int msg_type_id,
		const char* msg,
		const int msg_size)
	{
		cout << "Received msg type:" << msg_type_id << endl << "    ";
		for (int i = 0; i < msg_size; ++i)
		{
			cout << "0X" << hex << (unsigned int)(unsigned char)msg[i] << " ";
		}
		cout << endl;
	}
};

/*
test cases:


static int test_main(int argc,char* argv[])
{
	//case 1

	CHeadTailDatagramFilter filter3(1, "hi", "shit", 2, 4);
	CTxtDatagramFilterListener  listener;
	filter3.install_listener(&listener);
	filter3.input("aaaaahiairuqwruqw9r---1----eqsafasfshitbbbbbbbb");
	filter3.input("hiairuqwruqw9r---2----eqsafasfshit"，2);

	filter3.input("h");
	filter3.input("i");
	filter3<<'c';
	filter3 << 'i';
	filter3.input("-- 3 --");
	filter3.input("kishi");

	filter3.input("t", 2);

	// case 2
	CHeadTailDatagramFilter* filter = new CHeadTailDatagramFilter(2, "hi", "shit", 2, 4);
	CHeadTailDatagramFilter* filter1 = new CHeadTailDatagramFilter(3, "w9r", "eqsa", 3, 4);
	CDatagrammFilterMachine machine;
	machine.add_filter(filter);
	machine.add_filter(filter1);
	machine.install_listener(&listener);

	machine.input("hiairuqwruqw9r---1----eqsafasfshit");
	machine.input("hiairuqwruqw9r---2----eqsafasfshit");


	//=========================================================================
	// case 3
	// 处理gps报文
	CDatagrammFilterMachine machine2;
	CHeadTailDatagramFilter* filter5 = new CHeadTailDatagramFilter(4, "$GPRMC", "\r\n", 6, 2);
	CHeadTailDatagramFilter* filter6 = new CHeadTailDatagramFilter(5, "$GPGSV", "\r\n", 6, 2);
	machine2.add_filter(filter5);
	machine2.add_filter(filter6);
	machine2.install_listener(&listener);

	machine2 << ("a-gua\r\n$GPRMC,010101.130, A, 3606.6834, N, 12021.7778, E, 0.0, 238.3, 010807,,,A*6C\r\n");  //乱写的报文！！看能不能提出纯净的报文
	machine2 << ("gua-a,xxx$GPGSV，2，1，08，06，33，240，45，10，36，074，47，16，21，078，44，17，36，313，42*78\r\n");

	///处理流。。。。
	cout << "handle stream now ..." << endl;
	machine2 << "xxxxxxxxxxxxxxxxxxxxx$GPRMC,010101.130, A, 3606.6834, N,";
	machine2 << "0，45，10，36，074，47，16，21，078，44，17，36，313，42 * 78\r\n";

	cout << "-------- binary data processing ------------" << endl;
	char head[] = { 0XFF, 0XEF, 0X13, 0X2F };
	char tail[] = { '#', 0x10, 0x13 };
	int  head_len = sizeof(head) / sizeof(char);
	int  tail_len = sizeof(tail) / sizeof(char);

	// case 4, handle binary message!!
	CHeadTailDatagramFilter filter_1(0, head, tail, head_len, tail_len);
	CBinDatagramFilterListener bin_listener;
	filter_1.install_listener(&bin_listener);
	char msg_test[] = { 'a', 'g', 0x12, 0xbf, 0xff, 0xef, 0x13, 0x2f, 'K', 0x00, 0x12, 0x23, 0xA5, '#', 0x10, 0x13, 0x9E, 0x00 };
	filter_1.input(msg_test, sizeof(msg_test) / sizeof(char));

	return 0;
}


*/


#endif
