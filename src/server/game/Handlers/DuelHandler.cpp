/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "UpdateData.h"
#include "Player.h"
#include "Pet.h"

void WorldSession::HandleDuelAcceptedOpcode(WorldPacket& recvPacket)
{
    uint64 guid;
    Player* player;
    Player* plTarget;

    recvPacket >> guid;

    if (!GetPlayer()->duel)                                  // ignore accept from duel-sender
        return;

    player       = GetPlayer();
    plTarget = player->duel->opponent;

    if (player == player->duel->initiator || !plTarget || player == plTarget || player->duel->startTime != 0 || plTarget->duel->startTime != 0)
        return;

    //TC_LOG_DEBUG("network", "WORLD: Received CMSG_DUEL_ACCEPTED");
    TC_LOG_DEBUG("network", "Player 1 is: %u (%s)", player->GetGUIDLow(), player->GetName().c_str());
    TC_LOG_DEBUG("network", "Player 2 is: %u (%s)", plTarget->GetGUIDLow(), plTarget->GetName().c_str());

    time_t now = time(NULL);
    player->duel->startTimer = now;
    plTarget->duel->startTimer = now;

    // Reiniciar Cooldowns, vida y mana en una zona especifica.
    if (player->GetAreaId() == 4570 || player->GetAreaId() == 14 || player->GetAreaId() == 12)
    {
        player->SetHealth(player->GetMaxHealth());
        plTarget->SetHealth(plTarget->GetMaxHealth());
        player->RemoveArenaSpellCooldowns(true);
        plTarget->RemoveArenaSpellCooldowns(true);

	    // Debuffs
        player->RemoveAura(57723);
        player->RemoveAura(57724);
        player->RemoveAura(25771);
        player->RemoveAura(41425);
        player->RemoveAura(61987);
		player->RemoveAura(66233);
		player->RemoveAura(11196);
		player->RemoveAura(47986);
        plTarget->RemoveAura(57723);
        plTarget->RemoveAura(57724);
        plTarget->RemoveAura(25771);
        plTarget->RemoveAura(41425);
        plTarget->RemoveAura(61987);
        plTarget->RemoveAura(66233);
        plTarget->RemoveAura(11196);
		plTarget->RemoveAura(47986);

        if (player->getPowerType() == POWER_MANA)
            player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));
        if (plTarget->getPowerType() == POWER_MANA)
            plTarget->SetPower(POWER_MANA, plTarget->GetMaxPower(POWER_MANA));

    if (player->getPowerType() == POWER_RAGE)
        player->SetPower(POWER_RAGE, 0);
    if (plTarget->getPowerType() == POWER_RAGE)
        plTarget->SetPower(POWER_RAGE, 0);
    if (player->getPowerType() == POWER_RUNIC_POWER)
        player->SetPower(POWER_RUNIC_POWER, 0);
    if (plTarget->getPowerType() == POWER_RUNIC_POWER)
        plTarget->SetPower(POWER_RUNIC_POWER, 0);

        // Reset Pet Cooldown HP, Power, and Summon.
            if (player->getClass() == CLASS_HUNTER)
                player->CastSpell(player, 883, true);
        Pet* plPet = player->GetPet();
        if(plPet != NULL)
        {
            if (plPet->isDead())
                player->CastSpell(player, 35182, true);
				player->CastSpell(player, 982, true);
				player->RemoveAura(35182);
            plPet->SetHealth(plPet->GetMaxHealth());
            plPet->SetPower(plPet->getPowerType(), plPet->GetMaxPower(plPet->getPowerType()));
            plPet->RemoveArenaAuras();
			plPet->m_CreatureSpellCooldowns.clear();
			plPet->RemoveAura(55711);
        }

            if (plTarget->getClass() == CLASS_HUNTER)
                plTarget->CastSpell(plTarget, 883, true);
        Pet* plPetTarget = plTarget->GetPet();
        if(plPetTarget != NULL)
        {
            if (plPetTarget->isDead())
                plTarget->CastSpell(plTarget, 35182, true);
				plTarget->CastSpell(plTarget, 982, true);
				plTarget->RemoveAura(35182);
            plPetTarget->SetHealth(plPetTarget->GetMaxHealth());
            plPetTarget->SetPower(plPetTarget->getPowerType(), plPetTarget->GetMaxPower(plPetTarget->getPowerType()));
            plPetTarget->RemoveArenaAuras();
			plPetTarget->m_CreatureSpellCooldowns.clear();
			plPetTarget->RemoveAura(55711);
        }
	}

    player->SendDuelCountdown(3000);
    plTarget->SendDuelCountdown(3000);
}

void WorldSession::HandleDuelCancelledOpcode(WorldPacket& recvPacket)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_DUEL_CANCELLED");
    uint64 guid;
    recvPacket >> guid;

    // no duel requested
    if (!GetPlayer()->duel)
        return;

    // player surrendered in a duel using /forfeit
    if (GetPlayer()->duel->startTime != 0)
    {
        GetPlayer()->CombatStopWithPets(true);
        if (GetPlayer()->duel->opponent)
            GetPlayer()->duel->opponent->CombatStopWithPets(true);

        GetPlayer()->CastSpell(GetPlayer(), 7267, true);    // beg
        GetPlayer()->DuelComplete(DUEL_WON);
        return;
    }

    GetPlayer()->DuelComplete(DUEL_INTERRUPTED);
}
