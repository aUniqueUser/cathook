/*
 * hack.h
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#ifndef HACK_H_
#define HACK_H_

class IHack;
class CUserCmd;
class CViewSetup;
class bf_read;
class ConCommand;
class CCommand;

namespace hack {

extern bool shutdown;

void Initialize();
void Think();
void Shutdown();

void InitHacks();

void CC_Cat(const CCommand& args);
extern ConCommand* c_Cat;

}

#endif /* HACK_H_ */