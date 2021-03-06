//*****************************************************************************
//  All Winner Tech, All Right Reserved. 2006-2014 Copyright (c)
//
//  File name   :        di.c
//
//  Description :
//
//  History     :
//
//
//******************************************************************************

#include "di.h"
#include "di_ebios.h"

volatile __di_dev_t *di_dev;

//
//function: di_set_reg_base(unsigned int base)
//description: set di module register base
//parameters: base <di module AHB memory mapping >
//return   :
int di_set_reg_base(unsigned int base)
{
	di_dev = (__di_dev_t *)base;

	return 0;
}

//
//function: di_get_reg_base(void)
//description: get di module register base
//parameters:
//return   : di module AHB memory mapping
unsigned int di_get_reg_base(void)
{
	unsigned int ret = 0;

	ret = (unsigned int)(di_dev);

	return ret;
}

//
//function: di_set_init(void)
//description: set di module default register to ready de-interlace
//parameters:
//return   :
int di_set_init(__s32 mode)
{
	DI_Init(mode);

	return 0;
}

//
//function: di_reset(void)
//description: stop di module
//parameters:
//return   :
int di_reset(void)
{
	DI_Set_Reset();

	return 0;
}

//
//function: di_start(void)
//description: start a de-interlace function
//parameters:
//return   :
int di_start(void)
{
	DI_Set_Writeback_Start();

	return 0;
}

//
//function: di_irq_enable(unsigned int enable)
//description: enable/disable di irq
//parameters: enable <0-disable; 1-enable>
//return   :
int di_irq_enable(unsigned int enable)
{
	DI_Set_Irq_Enable(enable);

	return 0;
}

//
//function: di_get_status(void)
//description: get status of di module
//parameters:
//return  :  <0-Writeback finish; 1-Writeback no start; 2-Writeback-ing; (-1)-Undefined
int di_get_status(void)
{
	int ret;

	ret = DI_Get_Irq_Status();
	return ret;
}

//
//function: di_irq_clear()
//description: clear irq status
//parameters:
//return   :
int di_irq_clear(void)
{
	DI_Clear_irq();

	return 0;
}

//
//function: di_set_para(__disp_scaler_para_t *para)
//description: set parameters to ready a de-interlace function
//parameters: para <parameters which set from ioctrl>
//			  in_flag_add/out_flag_add <flag address malloc in driver>
//			  field <0 - select even line for source line; 1 - select odd line for source line>
//return  :   <0 - set OK; 1 - para NULL>
int di_set_para(__di_para_t *para, unsigned int in_flag_add, unsigned int out_flag_add, unsigned int field)
{
	__di_buf_addr_t in_addr;
	__di_buf_addr_t out_addr;
	__di_src_size_t in_size;
	__di_out_size_t out_size;
	__di_src_type_t in_type;
	__di_out_type_t out_type;
	__di_buf_addr_t pre_addr;

	if(para==NULL) {
		//DE_WRN("input parameter can't be null!\n");
		return -1;
	}

	in_type.fmt= di_sw_para_to_reg(0,para->input_fb.format);
	in_type.mod= di_sw_para_to_reg(1,para->input_fb.format);
	in_type.ps= di_sw_para_to_reg(2,para->input_fb.format);

	out_type.fmt = di_sw_para_to_reg(3,para->output_fb.format);
	out_type.ps = di_sw_para_to_reg(4,para->output_fb.format);
	out_type.alpha_en = 0;

	out_size.width  = para->out_regn.width;
	out_size.height = para->out_regn.height;
	out_size.fb_width = para->output_fb.size.width;
	out_size.fb_height = para->output_fb.size.height;

	in_addr.ch0_addr = (unsigned int )DI_VAtoPA((void*)(para->input_fb.addr[0]));
	in_addr.ch1_addr = (unsigned int )DI_VAtoPA((void*)(para->input_fb.addr[1]));

	in_size.src_width = para->input_fb.size.width;
	in_size.src_height = para->input_fb.size.height;
	in_size.scal_width= para->source_regn.width;
	in_size.scal_height= para->source_regn.height;

	out_addr.ch0_addr = (unsigned int )DI_VAtoPA((void*)(para->output_fb.addr[0]));
	out_addr.ch1_addr = (unsigned int )DI_VAtoPA((void*)(para->output_fb.addr[1]));

	pre_addr.ch0_addr = (unsigned int )DI_VAtoPA((void*)(para->pre_fb.addr[0]));
	pre_addr.ch1_addr = (unsigned int )DI_VAtoPA((void*)(para->pre_fb.addr[1]));

	DI_Module_Enable();

	DI_Config_Src(&in_addr,&in_size,&in_type);
	DI_Set_Scaling_Factor(&in_size, &out_size);
	DI_Set_Scaling_Coef(&in_size, &out_size, &in_type, &out_type);
	DI_Set_Out_Format(&out_type);
	DI_Set_Out_Size(&out_size);
	DI_Set_Writeback_Addr_ex(&out_addr,&out_size,&out_type);

	in_flag_add = (unsigned int )DI_VAtoPA((void*)in_flag_add);
	out_flag_add = (unsigned int )DI_VAtoPA((void*)out_flag_add);

	DI_Set_Di_PreFrame_Addr(pre_addr.ch0_addr, pre_addr.ch1_addr);
	DI_Set_Di_MafFlag_Src(in_flag_add, out_flag_add, 0x200);
	DI_Set_Di_Field(field);

	DI_Enable();
	DI_Set_Reg_Rdy();

	return 0;
}

// 0:deinterlace input pixel format
// 1:deinterlace input yuv mode
// 2:deinterlace input pixel sequence
// 3:deinterlace output format
// 4:deinterlace output pixel sequence
__s32 di_sw_para_to_reg(__u8 type, __u8 format)
{
	/* deinterlace input  pixel format */
	if(type == 0)	{
		if(format <= DI_FORMAT_MB32_21) {
			return DI_INYUV420;
		}	else {
			//DE_INF("not supported de-interlace input pixel format:%d in di_sw_para_to_reg\n",format);
		}
	}
	/* deinterlace input mode */
	else if(type == 1) {
		if((format == DI_FORMAT_MB32_21) || (format == DI_FORMAT_MB32_12)){
			return DI_UVCOMBINEDMB;
		}	else if((format == DI_FORMAT_NV21) || (format == DI_FORMAT_NV12)){
			return DI_UVCOMBINED;
		}	else {
			//DE_INF("not supported de-interlace input mode:%d in di_sw_para_to_reg\n", format);
		}
	}
	/* deinterlace input pixel sequence */
	else if(type == 2) {
		if((format == DI_FORMAT_MB32_12) || (format == DI_FORMAT_NV12)) {
			return DI_UVUV;
		}	else if((format == DI_FORMAT_MB32_21) || (format == DI_FORMAT_NV21)) {
			return DI_VUVU;
		}	else {
			//DE_INF("not supported de-interlace input pixel sequence:%d in di_sw_para_to_reg\n",format);
		}
	}
	/* deinterlace output format */
	else if(type == 3) {
		if((format == DI_FORMAT_NV12) || (format == DI_FORMAT_NV21)) {
			return DI_OUTUVCYUV420;
		}	else {
			//DE_INF("not supported de-interlace output format :%d in di_sw_para_to_reg\n", format);
		}
	}
	/*deinterlace output pixel sequence*/
	else if(type == 4) {
		if(format == DI_FORMAT_NV12) {
			return DI_UVUV;
		}	else if(format == DI_FORMAT_NV21) {
			return DI_VUVU;
		}	else{
			//DE_INF("not supported de-interlace output pixel sequence:%d in di_sw_para_to_reg\n", format);
		}
	}
	//DE_INF("not supported type:%d in di_sw_para_to_reg\n", type);
	return -1;
}

int di_internal_clk_enable(void)
{
	return DI_Internal_Set_Clk(1);
}

int di_internal_clk_disable(void)
{
	return DI_Internal_Set_Clk(0);
}
