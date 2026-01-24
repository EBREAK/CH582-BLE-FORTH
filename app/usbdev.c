#include "usbdev.h"
#include "CH58x_common.h"
#include "debug.h"
#include "fifo.h"

#include <assert.h>

volatile struct fifo8 usbdev_acm_forth_d2h_fifo = FIFO8_INIT(128);
volatile struct fifo8 usbdev_acm_forth_h2d_fifo = FIFO8_INIT(128);
volatile bool usbdev_acm_forth_h2d_disable = false;

volatile struct fifo8 usbdev_acm_data_d2h_fifo = FIFO8_INIT(256);
volatile struct fifo8 usbdev_acm_data_h2d_fifo = FIFO8_INIT(256);
volatile bool usbdev_acm_data_h2d_disable = false;

uint8_t *pEP5_RAM_Addr; //ep5_out(64)+ep5_in(64)
uint8_t *pEP6_RAM_Addr; //ep6_out(64)+ep6_in(64)
uint8_t *pEP7_RAM_Addr; //ep7_out(64)+ep7_in(64)

#define pEP5_OUT_DataBuf (pEP5_RAM_Addr)
#define pEP5_IN_DataBuf (pEP5_RAM_Addr + 64)
#define pEP6_OUT_DataBuf (pEP6_RAM_Addr)
#define pEP6_IN_DataBuf (pEP6_RAM_Addr + 64)
#define pEP7_OUT_DataBuf (pEP7_RAM_Addr)
#define pEP7_IN_DataBuf (pEP7_RAM_Addr + 64)

void DevEP5_OUT_Deal(uint8_t l);
void DevEP6_OUT_Deal(uint8_t l);
void DevEP7_OUT_Deal(uint8_t l);

// 设备描述符
const uint8_t MyDevDescr[] = {
	0x12, // bLength
	0x01, // bDescriptorType (Device)
	0x10, 0x01, // bcdUSB 1.10
	0xEF, // bDeviceClass
	0x02, // bDeviceSubClass
	0x01, // bDeviceProtocol
	0x40, // bMaxPacketSize0 64
	0x00, 0x00, // idVendor 0x0000
	0x00, 0x00, // idProduct 0x0000
	0x00, 0x00, // bcdDevice 0.00
	0x00, // iManufacturer (String Index)
	0x00, // iProduct (String Index)
	0x00, // iSerialNumber (String Index)
	0x01, // bNumConfigurations 1

	// 18 bytes
};
// 配置描述符
const uint8_t MyCfgDescr[] = {
	0x09, // bLength
	0x02, // bDescriptorType (Configuration)
	0x8D,
	0x00, // wTotalLength 141
	0x04, // bNumInterfaces 4
	0x01, // bConfigurationValue
	0x00, // iConfiguration (String Index)
	0x80, // bmAttributes
	0x45, // bMaxPower 138mA

	0x08, // bLength: 8
	0x0b, // bDescriptorType: 0x0b (INTERFACE ASSOCIATION)
	0x00, // bFirstInterface: 0
	0x02, // bInterfaceCount: 2
	0x02, // bFunctionClass: Communications and CDC Control (0x02)
	0x02, // bFunctionSubClass: 0x02
	0x01, // bFunctionProtocol: 0x01
	0x00, // iFunction: 0

	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x00, // bInterfaceNumber 0
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints 1
	0x02, // bInterfaceClass
	0x02, // bInterfaceSubClass
	0x01, // bInterfaceProtocol
	0x00, // iInterface (String Index)

	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x00, // Descriptor Subtype: Header Functional Descriptor (0x00)
	0x10,
	0x01, // CDC: 0x0110

	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x01, // Descriptor Subtype: Call Management Functional Descriptor (0x01)
	0x00, // bmCapabilities: 0x00
	// 0000 00.. = Reserved: 0x00
	// .... ..0. = Call Management over Data Class Interface: Not supported
	// .... ...0 = Call Management: Not supported
	0x01, // Data Interface: 0x01

	0x04, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x02, // Descriptor Subtype: Abstract Control Management Functional Descriptor (0x02)
	0x06, // bmCapabilities: 0x06
	// 0000 .... = Reserved: 0x0
	// .... 0... = Network_Connection: Not supported
	// .... .1.. = Send_Break: Supported
	// .... ..1. = Line Requests and State Notification: Supported
	// .... ...0 = Comm Features Combinations: Not supported
	//
	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x06, // Descriptor Subtype: Union Functional Descriptor (0x06)
	0x00, // Control Interface: 0x00
	0x01, // Subordinate Interface: 0x01

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x81, // bEndpointAddress (IN/D2H)
	0x03, // bmAttributes (Interrupt)
	0x40,
	0x00, // wMaxPacketSize 64
	0x01, // bInterval 1 (unit depends on device speed)

	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x01, // bInterfaceNumber 1
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints 2
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0x00, // bInterfaceProtocol
	0x00, // iInterface (String Index)

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x02, // bEndpointAddress (OUT/H2D)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x83, // bEndpointAddress (IN/D2H)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)

	0x08, // bLength: 8
	0x0b, // bDescriptorType: 0x0b (INTERFACE ASSOCIATION)
	0x02, // bFirstInterface: 2
	0x02, // bInterfaceCount: 2
	0x02, // bFunctionClass: Communications and CDC Control (0x02)
	0x02, // bFunctionSubClass: 0x02
	0x01, // bFunctionProtocol: 0x01
	0x00, // iFunction: 0

	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x02, // bInterfaceNumber 2
	0x00, // bAlternateSetting
	0x01, // bNumEndpoints 1
	0x02, // bInterfaceClass
	0x02, // bInterfaceSubClass
	0x01, // bInterfaceProtocol
	0x00, // iInterface (String Index)

	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x00, // Descriptor Subtype: Header Functional Descriptor (0x00)
	0x10,
	0x01, // CDC: 0x0110

	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x01, // Descriptor Subtype: Call Management Functional Descriptor (0x01)
	0x00, // bmCapabilities: 0x00
	// 0000 00.. = Reserved: 0x00
	// .... ..0. = Call Management over Data Class Interface: Not supported
	// .... ...0 = Call Management: Not supported
	0x01, // Data Interface: 0x01

	0x04, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x02, // Descriptor Subtype: Abstract Control Management Functional Descriptor (0x02)
	0x06, // bmCapabilities: 0x06
	// 0000 .... = Reserved: 0x0
	// .... 0... = Network_Connection: Not supported
	// .... .1.. = Send_Break: Supported
	// .... ..1. = Line Requests and State Notification: Supported
	// .... ...0 = Comm Features Combinations: Not supported
	//
	0x05, // bLength
	0x24, // bDescriptorType: 0x24 (CS_INTERFACE)
	0x06, // Descriptor Subtype: Union Functional Descriptor (0x06)
	0x02, // Control Interface: 0x02
	0x03, // Subordinate Interface: 0x03

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x85, // bEndpointAddress (IN/D2H)
	0x03, // bmAttributes (Interrupt)
	0x40,
	0x00, // wMaxPacketSize 64
	0x01, // bInterval 1 (unit depends on device speed)

	0x09, // bLength
	0x04, // bDescriptorType (Interface)
	0x03, // bInterfaceNumber 1
	0x00, // bAlternateSetting
	0x02, // bNumEndpoints 2
	0x0A, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0x00, // bInterfaceProtocol
	0x00, // iInterface (String Index)

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x06, // bEndpointAddress (OUT/H2D)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)

	0x07, // bLength
	0x05, // bDescriptorType (Endpoint)
	0x87, // bEndpointAddress (IN/D2H)
	0x02, // bmAttributes (Bulk)
	0x40,
	0x00, // wMaxPacketSize 64
	0x00, // bInterval 0 (unit depends on device speed)
};
// 语言描述符
const uint8_t MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };

/**********************************************************/
static uint8_t DevConfig;
static uint8_t SetupReqCode;
static uint16_t SetupReqLen;
static const uint8_t *pDescr;
static uint8_t usbdev_errflag = 0x00;
static uint8_t usbdev_len = 0x00;
static uint8_t usbdev_chtype = 0x00;

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(
	4))) uint8_t EP0_Databuf[64 + 64 + 64]; //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((
	aligned(4))) uint8_t EP1_Databuf[64 + 64]; //ep1_out(64)+ep1_in(64)
__attribute__((
	aligned(4))) uint8_t EP2_Databuf[64 + 64]; //ep2_out(64)+ep2_in(64)
__attribute__((
	aligned(4))) uint8_t EP3_Databuf[64 + 64]; //ep3_out(64)+ep3_in(64)
__attribute__((
	aligned(4))) uint8_t EP5_Databuf[64 + 64]; //ep5_out(64)+ep5_in(64)
__attribute__((
	aligned(4))) uint8_t EP6_Databuf[64 + 64]; //ep6_out(64)+ep6_in(64)
__attribute__((
	aligned(4))) uint8_t EP7_Databuf[64 + 64]; //ep7_out(64)+ep7_in(64)

__attribute__((always_inline)) static inline void
usbdev_handle_set_line_coding(void)
{
	usbdev_len = 0;
	debug_puts("USBDEV: SET LINE CODING\r\n");
}

__attribute__((always_inline)) static inline void
usbdev_handle_set_control_line_state(void)
{
	usbdev_len = 0;
	debug_puts("USBDEV: SET CONTROL LINE STATE\r\n");
}

__attribute__((always_inline)) static inline void
usbdev_handle_class_request(void)
{
	switch (SetupReqCode) {
	case 0x20:
		usbdev_handle_set_line_coding();
		break;
	case 0x22:
		usbdev_handle_set_control_line_state();
		break;
	default:
		debug_puts("USBDEV: UNHANDLE CLASS REQUEST ");
		debug_puthex(SetupReqCode);
		debug_puts("\r\n");
		usbdev_errflag = 0xFF;
	}
}
__attribute__((always_inline)) static inline void
usbdev_handle_vendor_reqeust(void)
{
	debug_puts("USBDEV: UNHANDLE VENDOR REQUEST\r\n");
	usbdev_errflag = 0xFF;
}

__attribute__((always_inline)) static inline void usbdev_handle_trans(void)
{
	switch (R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
	// 分析操作令牌和端点号
	{
	case UIS_TOKEN_IN: {
		switch (SetupReqCode) {
		case USB_GET_DESCRIPTOR:
			usbdev_len = SetupReqLen >= 0x40 ?
					     0x40 :
					     SetupReqLen; // 本次传输长度
			memcpy(pEP0_DataBuf, pDescr,
			       usbdev_len); /* 加载上传数据 */
			SetupReqLen -= usbdev_len;
			pDescr += usbdev_len;
			R8_UEP0_T_LEN = usbdev_len;
			R8_UEP0_CTRL ^= RB_UEP_T_TOG; // 翻转
			break;
		case USB_SET_ADDRESS:
			R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) |
					SetupReqLen;
			R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
			break;
		default:
			R8_UEP0_T_LEN =
				0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
			R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
			break;
		}
	} break;

	case UIS_TOKEN_OUT:
		usbdev_len = R8_USB_RX_LEN;
		break;

	case UIS_TOKEN_OUT | 1: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) { // 不同步的数据包将丢弃
			R8_UEP1_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP1_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 1:
		R8_UEP1_CTRL ^= RB_UEP_T_TOG;
		R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 2: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) { // 不同步的数据包将丢弃
			R8_UEP2_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP2_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 2:
		R8_UEP2_CTRL ^= RB_UEP_T_TOG;
		R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 3: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) { // 不同步的数据包将丢弃
			R8_UEP3_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP3_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 3:
		R8_UEP3_CTRL ^= RB_UEP_T_TOG;
		R8_UEP3_CTRL = (R8_UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 4: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) {
			R8_UEP4_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP4_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 4:
		R8_UEP4_CTRL ^= RB_UEP_T_TOG;
		R8_UEP4_CTRL = (R8_UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 5: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) {
			R8_UEP5_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP5_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 5:
		R8_UEP5_CTRL ^= RB_UEP_T_TOG;
		R8_UEP5_CTRL = (R8_UEP5_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 6: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) {
			R8_UEP6_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP6_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 6:
		R8_UEP6_CTRL ^= RB_UEP_T_TOG;
		R8_UEP6_CTRL = (R8_UEP6_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	case UIS_TOKEN_OUT | 7: {
		if (R8_USB_INT_ST & RB_UIS_TOG_OK) {
			R8_UEP7_CTRL ^= RB_UEP_R_TOG;
			usbdev_len = R8_USB_RX_LEN;
			DevEP7_OUT_Deal(usbdev_len);
		}
	} break;

	case UIS_TOKEN_IN | 7:
		R8_UEP7_CTRL ^= RB_UEP_T_TOG;
		R8_UEP7_CTRL = (R8_UEP7_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
		break;

	default:
		break;
	}
	R8_USB_INT_FG = RB_UIF_TRANSFER;
}

__attribute__((always_inline)) static inline void
usbdev_handle_standard_request(void)
{
	switch (SetupReqCode) {
	case USB_GET_DESCRIPTOR: {
		switch (((pSetupReqPak->wValue) >> 8)) {
		case USB_DESCR_TYP_DEVICE: {
			pDescr = MyDevDescr;
			usbdev_len = MyDevDescr[0];
			debug_puts("USBDEV: UPLOAD DEVICE DESCRIPTOR\r\n");
		} break;

		case USB_DESCR_TYP_CONFIG: {
			pDescr = MyCfgDescr;
			usbdev_len = MyCfgDescr[2];
			debug_puts("USBDEV: UPLOAD CONFIG DESCRIPTOR\r\n");
		} break;

		case USB_DESCR_TYP_STRING: {
			switch ((pSetupReqPak->wValue) & 0xff) {
			case 0:
				pDescr = MyLangDescr;
				usbdev_len = MyLangDescr[0];
				break;
			default:
				usbdev_errflag = 0xFF; // 不支持的字符串描述符
				break;
			}
		} break;

		default:
			usbdev_errflag = 0xff;
			break;
		}
		if (SetupReqLen > usbdev_len)
			SetupReqLen = usbdev_len; //实际需上传总长度
		usbdev_len = (SetupReqLen >= 0x40) ? 0x40 : SetupReqLen;
		memcpy(pEP0_DataBuf, pDescr, usbdev_len);
		pDescr += usbdev_len;
	} break;

	case USB_SET_ADDRESS:
		SetupReqLen = (pSetupReqPak->wValue) & 0xff;
		break;

	case USB_GET_CONFIGURATION:
		pEP0_DataBuf[0] = DevConfig;
		if (SetupReqLen > 1)
			SetupReqLen = 1;
		break;

	case USB_SET_CONFIGURATION:
		DevConfig = (pSetupReqPak->wValue) & 0xff;
		break;

	case USB_CLEAR_FEATURE: {
		if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) ==
		    USB_REQ_RECIP_ENDP) // 端点
		{
			switch ((pSetupReqPak->wIndex) & 0xff) {
			case 0x87:
				R8_UEP7_CTRL =
					(R8_UEP7_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x07:
				R8_UEP7_CTRL =
					(R8_UEP7_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x86:
				R8_UEP6_CTRL =
					(R8_UEP6_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x06:
				R8_UEP6_CTRL =
					(R8_UEP6_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x85:
				R8_UEP5_CTRL =
					(R8_UEP5_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x05:
				R8_UEP5_CTRL =
					(R8_UEP5_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x84:
				R8_UEP4_CTRL =
					(R8_UEP4_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x04:
				R8_UEP4_CTRL =
					(R8_UEP4_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x83:
				R8_UEP3_CTRL =
					(R8_UEP3_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x03:
				R8_UEP3_CTRL =
					(R8_UEP3_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x82:
				R8_UEP2_CTRL =
					(R8_UEP2_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x02:
				R8_UEP2_CTRL =
					(R8_UEP2_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			case 0x81:
				R8_UEP1_CTRL =
					(R8_UEP1_CTRL &
					 ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) |
					UEP_T_RES_NAK;
				break;
			case 0x01:
				R8_UEP1_CTRL =
					(R8_UEP1_CTRL &
					 ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) |
					UEP_R_RES_ACK;
				break;
			default:
				usbdev_errflag = 0xFF; // 不支持的端点
				break;
			}
		} else
			usbdev_errflag = 0xFF;
	} break;

	case USB_GET_INTERFACE:
		pEP0_DataBuf[0] = 0x00;
		if (SetupReqLen > 1)
			SetupReqLen = 1;
		break;

	case USB_GET_STATUS:
		pEP0_DataBuf[0] = 0x00;
		pEP0_DataBuf[1] = 0x00;
		if (SetupReqLen > 2)
			SetupReqLen = 2;
		break;

	default:
		usbdev_errflag = 0xff;
		break;
	}
}

__attribute__((always_inline)) static inline void usbdev_handle_setup(void)
{
	R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK |
		       UEP_T_RES_NAK;
	SetupReqLen = pSetupReqPak->wLength;
	SetupReqCode = pSetupReqPak->bRequest;
	usbdev_chtype = pSetupReqPak->bRequestType;

	usbdev_len = 0;
	usbdev_errflag = 0;
	if ((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) !=
	    USB_REQ_TYP_STANDARD) {
		switch (pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) {
		case USB_REQ_TYP_CLASS:
			usbdev_handle_class_request();
			break;
		case USB_REQ_TYP_VENDOR:
			usbdev_handle_vendor_reqeust();
			break;
		default:
			debug_puts("USBDEV: UNHANDLE REQEUST TYPE ");
			debug_puthex(pSetupReqPak->bRequestType &
				     USB_REQ_TYP_MASK);
			debug_puts("\r\n");
			usbdev_errflag = 0xFF;
		}
	} else /* 标准请求 */
	{
		usbdev_handle_standard_request();
	}
	if (usbdev_errflag == 0xff) // 错误或不支持
	{
		R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL |
			       UEP_T_RES_STALL; // STALL
		debug_puts("USBDEV: EP0 STALL\r\n");
	} else {
		if (usbdev_chtype & 0x80) // 上传
		{
			usbdev_len = (SetupReqLen > 0x40) ? 0x40 : SetupReqLen;
			SetupReqLen -= usbdev_len;
		} else {
			usbdev_len = 0; // 下传
		}
		R8_UEP0_T_LEN = usbdev_len;
		R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK |
			       UEP_T_RES_ACK; // 默认数据包是DATA1
	}

	R8_USB_INT_FG = RB_UIF_TRANSFER;
}

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB 传输处理函数
 *
 * @return  none
 */
void USB_DevTransProcess(void)
{
	uint8_t intflag;

	intflag = R8_USB_INT_FG;
	if (intflag & RB_UIF_TRANSFER) {
		if ((R8_USB_INT_ST & MASK_UIS_TOKEN) !=
		    MASK_UIS_TOKEN) // 非空闲
		{
			usbdev_handle_trans();
		}
		if (R8_USB_INT_ST & RB_UIS_SETUP_ACT) // Setup包处理
		{
			usbdev_handle_setup();
		}
	} else if (intflag & RB_UIF_BUS_RST) {
		R8_USB_DEV_AD = 0;
		R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
		R8_USB_INT_FG = RB_UIF_BUS_RST;
	} else if (intflag & RB_UIF_SUSPEND) {
		if (R8_USB_MIS_ST & RB_UMS_SUSPEND) {
			;
		} else {
			;
		}
		R8_USB_INT_FG = RB_UIF_SUSPEND;
	} else {
		R8_USB_INT_FG = intflag;
	}
}

void usbdev_init()
{
	pEP0_RAM_Addr = EP0_Databuf;
	pEP1_RAM_Addr = EP1_Databuf;
	pEP2_RAM_Addr = EP2_Databuf;
	pEP3_RAM_Addr = EP3_Databuf;
	pEP5_RAM_Addr = EP5_Databuf;
	pEP6_RAM_Addr = EP6_Databuf;
	pEP7_RAM_Addr = EP7_Databuf;
	R32_PB_PU &=
		~(GPIO_Pin_11 | GPIO_Pin_10); // use usb controller's pullup
	R16_UEP0_DMA = (uint16_t)(uint32_t)pEP0_RAM_Addr;
	R8_USB_CTRL = 0x00; // 先设定模式,取消 RB_UC_CLR_ALL

	R8_UEP4_1_MOD = RB_UEP4_RX_EN | RB_UEP4_TX_EN | RB_UEP1_RX_EN |
			RB_UEP1_TX_EN;
	R8_UEP2_3_MOD = RB_UEP2_RX_EN | RB_UEP2_TX_EN | RB_UEP3_RX_EN |
			RB_UEP3_TX_EN;
	R8_UEP567_MOD = RB_UEP5_RX_EN | RB_UEP5_TX_EN | RB_UEP6_RX_EN |
			RB_UEP6_TX_EN | RB_UEP7_RX_EN | RB_UEP7_TX_EN;
	R16_UEP1_DMA = (uint16_t)(uint32_t)pEP1_RAM_Addr;
	R16_UEP2_DMA = (uint16_t)(uint32_t)pEP2_RAM_Addr;
	R16_UEP3_DMA = (uint16_t)(uint32_t)pEP3_RAM_Addr;
	R16_UEP5_DMA = (uint16_t)(uint32_t)pEP5_RAM_Addr;
	R16_UEP6_DMA = (uint16_t)(uint32_t)pEP6_RAM_Addr;
	R16_UEP7_DMA = (uint16_t)(uint32_t)pEP7_RAM_Addr;

	R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
	R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
	R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
	R8_UEP4_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
	R8_UEP5_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
	R8_UEP6_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
	R8_UEP7_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;

	R8_USB_DEV_AD = 0x00;
	R8_USB_CTRL = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN;
	R16_PIN_ANALOG_IE |= RB_PIN_USB_IE | RB_PIN_USB_DP_PU;
	R8_USB_INT_FG = 0xFF;
	R8_UDEV_CTRL = RB_UD_PD_DIS | RB_UD_PORT_EN;
	R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;
	PFIC_EnableIRQ(USB_IRQn);
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   端点1数据处理
 *
 * @return  none
 */
void DevEP1_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      DevEP2_OUT_Deal
 *
 * @brief   端点2数据处理
 *
 * @return  none
 */
void DevEP2_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
	int space_avail;
	space_avail = fifo8_num_free(&usbdev_acm_forth_h2d_fifo);
	assert(space_avail >= 64);
	assert(usbdev_acm_forth_h2d_disable == false);

	fifo8_push_all(&usbdev_acm_forth_h2d_fifo, pEP2_OUT_DataBuf, l);
	space_avail = fifo8_num_free(&usbdev_acm_forth_h2d_fifo);

#if 1
	debug_puts("USBDEV: EP2 OUT 0x");
	debug_puthex(l);
	debug_puts(" BYTES\r\n");
#endif
	if (space_avail < 64) {
		debug_puts("USBDEV: EP2 FLOW CONTROL TRIG\r\n");
		R8_UEP2_CTRL = UEP_R_RES_NAK;
	}
}

/*********************************************************************
 * @fn      DevEP3_OUT_Deal
 *
 * @brief   端点3数据处理
 *
 * @return  none
 */
void DevEP3_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      DevEP4_OUT_Deal
 *
 * @brief   端点4数据处理
 *
 * @return  none
 */
void DevEP4_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
}

void DevEP5_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
}

void DevEP6_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
	int space_avail;
	space_avail = fifo8_num_free(&usbdev_acm_data_h2d_fifo);
	assert(space_avail >= 64);
	assert(usbdev_acm_data_h2d_disable == false);

	fifo8_push_all(&usbdev_acm_data_h2d_fifo, pEP6_OUT_DataBuf, l);
	space_avail = fifo8_num_free(&usbdev_acm_data_h2d_fifo);

#if 1
	debug_puts("USBDEV: EP6 OUT 0x");
	debug_puthex(l);
	debug_puts(" BYTES\r\n");
#endif
	if (space_avail < 64) {
		debug_puts("USBDEV: EP6 FLOW CONTROL TRIG\r\n");
		R8_UEP6_CTRL = UEP_R_RES_NAK;
	}
}

void DevEP7_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void USB_IRQHandler(void) /* USB中断服务程序,使用寄存器组1 */
{
	USB_DevTransProcess();
}
