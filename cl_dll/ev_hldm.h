//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

void EV_HLDM_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName);
void EV_HLDM_DecalGunshot(pmtrace_t* pTrace, int iBulletType);
void EV_HLDM_CheckTracer(int idx, float* vecSrc, float* end, float* forward, float* right, int iBulletType, int iTracerFreq, int* tracerCount);
void EV_HLDM_FireBullets(int idx, float* forward, float* right, float* up, int cShots, float* vecSrc, float* vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int* tracerCount, float flSpreadX, float flSpreadY);

// HLDM
void EV_FireMP5(event_args_t* args);
void EV_FireFiftyCal(event_args_t* args);
void EV_Sledgehammer(event_args_t* args);


void EV_TrainPitchAdjust(event_args_t* args);
void EV_VehiclePitchAdjust(event_args_t* args);
