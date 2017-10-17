#pragma once
#include <vector>
#include <SDL2/SDL.h>

#include <hydra/ext/api.hpp>

enum Buffs {
	BUFF_HEALING,
	BUFF_HEALTHUPGRADE,
	BUFF_DAMAGEUPGRADE,
	BUFF_BULLETVELOCITY
};

enum Debuffs {
	DEBUFF_POISON,
	DEBUFF_BURNING
};

class HYDRA_PHYSICS_API BuffHandler {
public:
	BuffHandler();
	~BuffHandler();
	std::vector<Buffs> getActiveBuffs();
	void getActiveDebuffs();
	bool addBuff(Buffs newBuff);
	void onActivation(int &maxHealth, int &health);
	void onTick(int &maxHealth, int &health);
	void onAttack(float &bulletVelocity);
private:
	std::vector<Buffs> _activeBuffs;
	std::vector<Debuffs> _activeDebuffs;
	std::vector<Uint32> _activeBuffsTimer;
	size_t _lastPickedUpBuff;
};