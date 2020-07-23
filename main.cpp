#include "datagram.h"

/*
   in the following test case, I used a pre-defined listener for message processing,
   you can define your listener to define your expected behavior.
   
   zxg519@sina.com
   SEU, China

*/


int main(int argc, char* argv[])
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
