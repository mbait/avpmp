#ifndef _ltfx_exp_h
#define _ltfx_exp_h 1

#ifdef __cplusplus

extern "C"
{

#endif

typedef enum 
{

	LFX_RandomFlicker,
	LFX_Strobe,
	LFX_Switch,
	LFX_FlickySwitch,
	
} LIGHT_FX_TYPE;

typedef enum
{
	
	LFXS_LightOn,
	LFXS_LightOff,
	LFXS_LightFadingUp,
	LFXS_LightFadingDown,
	LFXS_Flicking,
	LFXS_NotFlicking,
	
} LIGHT_FX_STATE;

typedef struct
{
	
	unsigned int type;
	unsigned int init_state;
	
	unsigned int fade_up_speed;
	unsigned int fade_down_speed;
	
	unsigned int post_fade_up_delay;
	unsigned int post_fade_down_delay;

} LightFXData;

#ifdef __cplusplus

};

#endif

#endif
