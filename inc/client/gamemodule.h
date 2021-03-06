/*
// LICENSE HERE.

//
// gamemodule.h
//
// Contains declarations of all the functions concerning interaction with
// the client game module.
//
*/
#ifndef __CLIENT_CLGMODULE_H__
#define __CLIENT_CLGMODULE_H__

//
// cgmodule.c
//
void CL_ShutdownGameProgs(void);    // N&C: Place elsewhere?
extern "C" void CL_InitGameProgs(void);        // N&C: Place elsewhere? // CPP: Extern "C"

//
// Core
//
void        CL_GM_Init();
void        CL_GM_Shutdown(void);

float		CL_GM_CalcFOV(float fov_x, float width, float height);
void		CL_GM_CalcViewValues(void);
void		CL_GM_ClientBegin(void);
void		CL_GM_ClientDeltaFrame(void);
void		CL_GM_ClientFrame(void);
void		CL_GM_ClientDisconnect(void);
void		CL_GM_ClearState(void);
void		CL_GM_DemoSeek(void);
void		CL_GM_UpdateUserInfo(cvar_t* var, from_t from);

//
// Entities.
//
void		CL_GM_EntityEvent(int number);

//
// Media
//
void        CL_GM_InitMedia(void);
const char	*CL_GM_GetMediaLoadStateName(load_state_t state);
void        CL_GM_LoadScreenMedia(void);
void        CL_GM_LoadWorldMedia(void);
void        CL_GM_ShutdownMedia(void);

//
// PMove.
//
void		CL_GM_PMoveInit(pmoveParams_t* pmp);
void		CL_GM_PMoveEnableQW(pmoveParams_t* pmp);

//
// Predict
//
void		CL_GM_CheckPredictionError(int frame, unsigned int cmd);
void		CL_GM_PredictAngles(void);
void		CL_GM_PredictMovement(unsigned int ack, unsigned int current);

//
// Parse
//
qboolean	CL_GM_UpdateConfigString(int index, const char* str);
void		CL_GM_StartServerMessage(void);
qboolean	CL_GM_ParseServerMessage(int serverCommand);
qboolean    CL_GM_SeekDemoMessage(int demoCommand);
void		CL_GM_EndServerMessage(void);

//
// Screen
//
void		CL_GM_RenderScreen(void);
void		CL_GM_ScreenModeChanged(void);

void		CL_GM_DrawLoadScreen(void);
void		CL_GM_DrawPauseScreen(void);

//
// View
//
void		CL_GM_ClearScene(void);

void		CL_GM_PreRenderView(void);
void		CL_GM_RenderView(void);
void		CL_GM_PostRenderView(void);

#endif //  __CLIENT_CGMODULE_H__