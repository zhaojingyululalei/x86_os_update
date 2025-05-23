#include "dev/kbd.h"
#include "irq/irq.h"
#include "printk.h"
#include "types.h"
#include "cpu_instr.h"
#include "dev/tty.h"
extern void exception_handler_kbd(void);
static kbd_state_t kbd_state;	// 键盘状态
static const key_map_t map_table[256] = {
        [0x2] = {'1', '!'},
        [0x3] = {'2', '@'},
        [0x4] = {'3', '#'},
        [0x5] = {'4', '$'},
        [0x6] = {'5', '%'},
        [0x7] = {'6', '^'},
        [0x08] = {'7', '&'},
        [0x09] = {'8', '*' },
        [0x0A] = {'9', '('},
        [0x0B] = {'0', ')'},
        [0x0C] = {'-', '_'},
        [0x0D] = {'=', '+'},
        [0x0E] = {ASCII_DEL, ASCII_DEL},
        [0x0F] = {'\t', '\t'},//tab
        [0x10] = {'q', 'Q'},
        [0x11] = {'w', 'W'},
        [0x12] = {'e', 'E'},
        [0x13] = {'r', 'R'},
        [0x14] = {'t', 'T'},
        [0x15] = {'y', 'Y'},
        [0x16] = {'u', 'U'},
        [0x17] = {'i', 'I'},
        [0x18] = {'o', 'O'},
        [0x19] = {'p', 'P'},
        [0x1A] = {'[', '{'},
        [0x1B] = {']', '}'},
        [0x1C] = {'\n', '\n'},//回车
        [0x1E] = {'a', 'A'},
        [0x1F] = {'s', 'B'},
        [0x20] = {'d',  'D'},
        [0x21] = {'f', 'F'},
        [0x22] = {'g', 'G'},
        [0x23] = {'h', 'H'},
        [0x24] = {'j', 'J'},
        [0x25] = {'k', 'K'},
        [0x26] = {'l', 'L'},
        [0x27] = {';', ':'},
        [0x28] = {'\'', '"'},
        [0x29] = {'`', '~'},
        [0x2B] = {'\\', '|'},
        [0x2C] = {'z', 'Z'},
        [0x2D] = {'x', 'X'},
        [0x2E] = {'c', 'C'},
        [0x2F] = {'v', 'V'},
        [0x30] = {'b', 'B'},
        [0x31] = {'n', 'N'},
        [0x32] = {'m', 'M'},
        [0x33] = {',', '<'},
        [0x34] = {'.', '>'},
        [0x35] = {'/', '?'},
        [0x39] = {' ', ' '}, //空格
};
/*获取低7位*/
static inline char get_key(uint8_t key_code) {
    return key_code & 0x7F;
}
/*判断是否是make_code*/
static inline int is_make_code(uint8_t key_code) {
    return !(key_code & 0x80);
}

static void do_fx_key (int key) {
    int index = key - KEY_F1;
    if (kbd_state.lctrl_press || kbd_state.rctrl_press) {
        tty_select(index);
    }
}
/**
 * 处理单字符的标准键
 */
static void do_normal_key (uint8_t raw_code) {
    char key = get_key(raw_code);		// 去掉最高位
    int is_make = is_make_code(raw_code);

    // 暂时只处理按键按下
	switch (key) {
	    // shift, alt, ctrl键，记录标志位
	case KEY_RSHIFT:
		kbd_state.rshift_press = is_make;  // 仅设置标志位
		break;
	case KEY_LSHIFT:
		kbd_state.lshift_press = is_make;  // 仅设置标志位
		break;
    case KEY_CAPS:  // 大小写键，设置大小写状态
		if (is_make) {
			kbd_state.caps_lock = ~kbd_state.caps_lock;
		}
		break;
    case KEY_ALT:
        kbd_state.lalt_press = is_make;  // 仅设置标志位
        break;
    case KEY_CTRL:
        kbd_state.lctrl_press = is_make;  // 仅设置标志位
        break;
    // 功能键：写入键盘缓冲区，由应用自行决定如何处理
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
         do_fx_key(key);
        break;
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_SCROLL_LOCK:
    default:
        if (is_make) {
            // 根据shift控制取相应的字符，这里有进行大小写转换或者shif转换
            if (kbd_state.rshift_press || kbd_state.lshift_press) {
                key = map_table[key].func;  // 第2功能
            }else {
                key = map_table[key].normal;  // 第1功能
            }

            // 根据caps再进行一次字母的大小写转换
            if (kbd_state.caps_lock) {
                if ((key >= 'A') && (key <= 'Z')) {
                    // 大写转小写
                    key = key - 'A' + 'a';
                } else if ((key >= 'a') && (key <= 'z')) {
                    // 小写转大小
                    key = key - 'a' + 'A';
                }
            }

            
            tty_in(key);
            
        }
        break;
    }
}
/**
 * E0开始的键处理，只处理功能键，其它更长的序列不处理
 */
static void do_e0_key (uint8_t raw_code) {
    int key = get_key(raw_code);			// 去掉最高位
    int is_make = is_make_code(raw_code);	// 按下或释放

    // E0开头，主要是HOME、END、光标移动等功能键
    // 设置一下光标位置，然后直接写入
    switch (key) {
        case KEY_CTRL:		// 右ctrl和左ctrl都是这个值
            kbd_state.rctrl_press = is_make;  // 仅设置标志位
            break;
        case KEY_ALT:
            kbd_state.ralt_press = is_make;  // 仅设置标志位
            break;
    }
}


void do_handler_kbd(exception_frame_t *frame) {
    static enum {
    	NORMAL,				// 普通，无e0或e1
		BEGIN_E0,			// 收到e0字符
		BEGIN_E1,			// 收到e1字符
    }recv_state = NORMAL;
    // 检查是否有数据，无数据则退出
	uint8_t status = inb(KBD_PORT_STAT);
	if (!(status & KBD_STAT_RECV_READY)) {
        pic_send_eoi(IRQ1_KEYBOARD);
		return;
	}

	// 读取键值
    uint8_t raw_code = inb(KBD_PORT_DATA);

	// 读取完成之后，就可以发EOI，方便后续继续响应键盘中断
	// 否则,键值的处理过程可能略长，将导致中断响应延迟
    pic_send_eoi(IRQ1_KEYBOARD);
    
    
    if (raw_code == KEY_E0) {
        //如果是E0码开头的字符
		recv_state = BEGIN_E0;
	} else if (raw_code == KEY_E1) {
		// E1字符，不处理
		recv_state = BEGIN_E1;
	} else {
		switch (recv_state) {
		case NORMAL:
			do_normal_key(raw_code);
			break;
		case BEGIN_E0: // 不处理print scr
			do_e0_key(raw_code);
			recv_state = NORMAL;
			break;
		case BEGIN_E1:  // 不处理pause
			recv_state = NORMAL;
			break;
		}
	}
}


void kbd_init(void){
    interupt_install(IRQ1_KEYBOARD,exception_handler_kbd);
    irq_enable(IRQ1_KEYBOARD);
}