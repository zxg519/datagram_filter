
#include "datagram.h"
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDatagramFilter::CDatagramFilter(int msg_type_id,  // 报文组别！如果没有多组报文，可以不关心，这个参数会被传递到CDatagramFilterListener里面的回掉函数中
	CDatagramFilterListener* pistener           // 事件侦听器，如果匹配成功，回调CDatagramFilterListener::OnValidMessage()
	) :m_msg_type_id(msg_type_id), m_plistener(pistener)
{
}



CDatagramFilter::WORK_STATE CDatagramFilter::input(const char * msg, int length)
{
	// 计算串长
	(length == 0) ? (length = strlen(msg)) : (length); // 如果默认参数，认为字符串

	// 逐个处理
	WORK_STATE ret = CHECKING_HEADER;
	for (int i = 0; i < length; ++i)
		ret = input_inner(msg[i]);

	// 返回
	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CDatagrammFilterMachine::add_filter(CDatagramFilter* p_filter)
{
	m_array.push_back(p_filter);
}

CDatagrammFilterMachine& CDatagrammFilterMachine::operator << (const char c)
{
	input(c);
	return *this;
}

CDatagrammFilterMachine& CDatagrammFilterMachine::operator << (const char* str)
{
	input(str, strlen(str));
	return *this;
}

void CDatagrammFilterMachine::input(const char * msg, int length)
{
	int len = m_array.size();
	(length == 0) ? (length = strlen(msg)) : (length); // 如果默认参数，认为字符串, 计算串长

	for (int i = 0; i < len; ++i)
		(*m_array[i]).input(msg, length);
}
void CDatagrammFilterMachine::reset()
{
	int len = m_array.size();
	for (int i = 0; i < len; ++i)
		(*m_array[i]).reset();
}

void CDatagrammFilterMachine::input(const char c)
{
	int len = m_array.size();
	for (int i = 0; i < len; ++i)
		m_array[i]->input(c);
}


void CDatagrammFilterMachine::install_listener(CDatagramFilterListener* plistener)
{
	int len = m_array.size();
	for (int i = 0; i < len; ++i)
		m_array[i]->install_listener(plistener);
}

CDatagrammFilterMachine::~CDatagrammFilterMachine()
{
	int len = m_array.size();
	for (int i = 0; i < len; ++i)
	{
		delete m_array[i];
		m_array[i] = NULL;
	}

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CHeadTailDatagramFilter::CHeadTailDatagramFilter(int msg_type_id,
	const char* p_head,
	const char* p_tail,
	int head_length,
	int tail_length) : m_state(CHECKING_HEADER), m_sBuffer(2000), m_cursor(0), CDatagramFilter(msg_type_id, NULL)
{
	(head_length <= 0) ? (head_length = strlen(p_head)) : (head_length);
	(tail_length <= 0) ? (tail_length = strlen(p_tail)) : (tail_length);

	int i = 0;
	// 为保证和二进制的兼容，不要直接字符串赋值！！！
	for (i = 0; i < head_length; ++i)
		m_header.push_back(p_head[i]);
	for (i = 0; i < tail_length; ++i)
		m_tail.push_back(p_tail[i]);

	m_header_len = head_length;
	m_tail_len = tail_length;

	m_sBuffer.clear();
}

#define ON_ERROR(msg)\
	    if (this->m_plistener) \
		     m_plistener->on_error_msg("\7\7\7Error: impossible case!!!")

/* virtual*/
CDatagramFilter::WORK_STATE CHeadTailDatagramFilter::input_inner(const char c)
{
	switch (m_state)
	{

	case READ_RESET:
		reset();	// reset all information
		m_state = CHECKING_HEADER;

	case CHECKING_HEADER:	// check signal header
		if (m_cursor <= m_header_len - 1)
		{
			if (c != m_header[m_cursor])
			{
				reset();
				return m_state;
			}

			if ((++m_cursor) == m_header_len)
			{
				m_cursor = 0;
				return m_state = CHECKING_TAIL;	// next state = try to match tail
			}
			else
				return m_state = CHECKING_HEADER;
		}
		else
		{
			ON_ERROR("\7\7\7Error: impossible case!!!");
			reset();
			return m_state;
		}
		break;

	case CHECKING_TAIL:      // CHECKING MESSAGE TAIL
		if (m_cursor <= m_tail_len - 1)
		{
			this->m_sBuffer.push_back(c);
			if (c != m_tail[m_cursor])
			{
				m_cursor = 0;
				return m_state = CHECKING_TAIL;
			}

			if ((++m_cursor) == m_tail_len)
			{
				m_sBuffer[m_sBuffer.size() - m_tail_len] = 0;
				if (m_plistener)
					m_plistener->OnValidMessage(get_msg_type_id(), &(m_sBuffer[0]), m_sBuffer.size() - m_tail_len);

				reset();
				return m_state = CHECKING_HEADER;
			}
			return m_state = CHECKING_TAIL;
		}
		else
		{
			ON_ERROR("\7\7\7Error: impossible case!!!");
			reset();
			return m_state = CHECKING_HEADER;
		}

		break;
	default:
		ON_ERROR("\7\7\7Error: bad internal FSM state!!!");
		reset();
		return m_state;
	}

}
//WORK_STATE input(const char * msg, int length = 0)

/* virtual */
void CHeadTailDatagramFilter::reset()
{
	m_cursor = 0;
	m_state = CHECKING_HEADER;
	m_sBuffer.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFixSizedDatagramFilter::CFixSizedDatagramFilter(int msg_type_id,
	const char* p_head,
	int head_length,
	int datagram_length,
	const char* p_tail,   // 支持核对报文尾，如果输入为0，则不支持！
	int tail_length) : m_state(CHECKING_HEADER), m_sBuffer(2000), m_cursor(0), CDatagramFilter(msg_type_id, NULL)
{
	(head_length <= 0) ? (head_length = strlen(p_head)) : (head_length);
	if (p_tail)
		(tail_length <= 0) ? (tail_length = strlen(p_tail)) : (tail_length);
	else
		tail_length = 0;
	m_datagram_length = datagram_length;

	assert(m_datagram_length > 0);
	assert(head_length > 0);
	assert(p_head);

	int i = 0;
	// 为保证和二进制的兼容，不要直接字符串赋值！！！
	for (i = 0; i < head_length; ++i)
		m_header.push_back(p_head[i]);
	for (i = 0; i < tail_length; ++i)
		m_tail.push_back(p_tail[i]);

	m_header_len = head_length;
	m_tail_len = tail_length;

	m_sBuffer.clear();
}


/*virtual */
void CFixSizedDatagramFilter::reset()
{
	m_cursor = 0;
	m_state = CHECKING_HEADER;
	m_sBuffer.clear();
}
	// 实现处理数据流的核心逻辑，被operator <<, input()函数调用！！！
/* virtual */
CDatagramFilter::WORK_STATE CFixSizedDatagramFilter::input_inner(const char c)
{
	switch (m_state)
	{

	case READ_RESET:
		reset();	// reset all information
		m_state = CHECKING_HEADER;

	case CHECKING_HEADER:	// check signal header
		if (c != m_header[m_cursor])
		{
			// 如果失配，依据KMP原理，我们的报文头尾调用的前提是不具有相似性，因此可以直接重置状态！！！
			reset();
			return m_state;
		}

		if ((++m_cursor) == m_header_len)
		{
			m_cursor = 0;
			m_state = READING_DATA;	// next state = try to match tail
		}
		return m_state;
		break;

	case READING_DATA:
		// 
		m_sBuffer.push_back(c);
		if (m_sBuffer.size() == m_datagram_length)
		{
			if (m_tail.size() == 0)
			{
				if (m_plistener)
					m_plistener->OnValidMessage(get_msg_type_id(), &(m_sBuffer[0]), m_sBuffer.size());
				reset();
				return m_state = CHECKING_HEADER;
			}
			else
			{
				return m_state = CHECKING_TAIL;
			}
		}
		return m_state;
		break;
	case CHECKING_TAIL:      // CHECKING MESSAGE TAIL
		if (m_cursor <= m_tail_len - 1)
		{
			this->m_sBuffer.push_back(c);
			if (c != m_tail[m_cursor])
			{
				// 报文尾错误，舍弃这组报文
				reset();
				return m_state;
			}

			if ((++m_cursor) == m_tail_len)
			{
				m_sBuffer[m_sBuffer.size() - m_tail_len] = 0;
				if (m_plistener)
					m_plistener->OnValidMessage(get_msg_type_id(), &(m_sBuffer[0]), m_sBuffer.size() - m_tail_len);

				reset();
				return m_state;
			}
			return m_state = CHECKING_TAIL;
		}
		else
		{
			ON_ERROR("\7\7\7error in message tail!!!");
			reset();
			return m_state;
		}

		break;
	default:
		ON_ERROR("\7\7\7Error: impossible case!!!");
		reset();
		return m_state;
	}

	ON_ERROR("\7\7\7Error: bad internal FSM state!!!");
	reset();
	return m_state;
}

#ifndef _CONSOLE
    static int test_main(int argc, _TCHAR* argv[])
#else
    static int main(int argc, _TCHAR* argv[])
#endif
{
		CTxtDatagramFilterListener  listener;
#if 0
	/*
	case 1
	*/

	CHeadTailDatagramFilter filter3(1, "hi", "shit", 2, 4);
	CTxtDatagramFilterListener  listener;
	filter3.install_listener(&listener);
	/*
	filter3.input("aaaaahiairuqwruqw9r---1----eqsafasfshitbbbbbbbb");
	filter3.input("hiairuqwruqw9r---2----eqsafasfshit"，2);

	filter3.input("h");
	filter3.input("i");
	filter3<<'c';
	filter3 << 'i';
	filter3.input("-- 3 --");
	filter3.input("kishi");
	*/
	filter3.input("t", 2);


	CHeadTailDatagramFilter* filter = new CHeadTailDatagramFilter(2, "hi", "shit", 2, 4);
	CHeadTailDatagramFilter* filter1 = new CHeadTailDatagramFilter(3, "w9r", "eqsa", 3, 4);
	CDatagrammFilterMachine machine;
	machine.add_filter(filter);
	machine.add_filter(filter1);
	machine.install_listener(&listener);

	machine.input("hiairuqwruqw9r---1----eqsafasfshit");
	machine.input("hiairuqwruqw9r---2----eqsafasfshit");


	//=========================================================================
	// 处理gps报文
	CDatagrammFilterMachine machine2;
	CHeadTailDatagramFilter* filter5 = new CHeadTailDatagramFilter(4, "$GPRMC", "\r\n", 6, 2);
	CHeadTailDatagramFilter* filter6 = new CHeadTailDatagramFilter(5, "$GPGSV", "\r\n", 6, 2);
	machine2.add_filter(filter5);
	machine2.add_filter(filter6);
	machine2.install_listener(&listener);

	machine2 << ("a-gua\r\n$GPRMC,010101.130, A, 3606.6834, N, 12021.7778, E, 0.0, 238.3, 010807,,,A*6C\r\n$GPRMC...");  //乱写的报文！！看能不能提出纯净的报文
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

	CHeadTailDatagramFilter filter_1(0, head, tail, head_len, tail_len);
	CBinDatagramFilterListener bin_listener;
	filter_1.install_listener(&bin_listener);
	char msg_test[] = { 'a', 'g', 0x12, 0xbf, 0xff, 0xef, 0x13, 0x2f, 'K', 0x00, 0x12, 0x23, 0xA5, '#', 0x10, 0x13, 0x9E, 0x00 };
	filter_1.input(msg_test, sizeof(msg_test) / sizeof(char));
#endif
	///-------------------------------------------------------------------------------------
	char head100[] = "$GPGGA";
	int  head_len = strlen(head100);
	int  msg_length = 20;

	// 这个过滤器，处理定长报文，但会校验报文尾，如果报文尾不符合直接丢弃！！！
	CFixSizedDatagramFilter filter100(0,head100,head_len,msg_length,"\r\n");
	filter100.install_listener(&listener);
	filter100 << "xxx?$GPGGA12345678901234567890\r\n$GPGGA123456789012345678901\r\n$GPGGA12342323235678901234567892323232300\r\n";

	// 下面的报文过滤器不管报文尾
	CFixSizedDatagramFilter filter101(1, head100, head_len, msg_length);
	filter101.install_listener(&listener);
	filter101 << "xxx?$GPGGA12345678901234567890\r\n$GPGGA123456789012345678901\r\n$GPGGA12342323235678901234567892323232300\r\n";


	return 0;
}
