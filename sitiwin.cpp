#include "sitwin.hpp"
#include "singleton.hpp"
#include "const/character.hpp"

SitWin::SitWin(short item_id, short gameworld_id, short item_amount)
{
    this->run = false;
    this->play = false;
    this->item_id = item_id;
    this->item_amount = item_amount;
    this->gameworld_id = gameworld_id;
    this->winner = -1;
    this->clock.restart();
}

SitWin::SitWin()
{
    this->run = false;
    this->play = false;
    this->item_id = 0;
    this->item_amount = 0;
    this->gameworld_id = -1;
    this->winner = -1;
    this->clock.restart();
    this->reminder_clock.restart();
}

void SitWin::Run(short gameworld_id)
{
    this->run = true;
    this->play = false;
    this->item_id = 0;
    this->item_amount = 0;
    this->gameworld_id = gameworld_id;
    this->winner = -1;
    this->clock.restart();
    this->reminder_clock.restart();

    S::GetInstance().eoclient.TradeRequest(this->gameworld_id);
    S::GetInstance().eoclient.Sit(SitAction::Sit);
}

void SitWin::Process()
{
    S &s = S::GetInstance();

    if(this->run)
    {
        if(this->play)
        {
            this->play = false;

            std::vector<Character> winners;
            for(unsigned int i = 0; i < s.map.characters.size(); ++i)
            {
                int distance = path_length(s.map.characters[i].x, s.map.characters[i].y, s.character.x, s.character.y);
                if(distance <= 1)
                {
                    if(s.map.characters[i].gameworld_id != s.character.gameworld_id && s.map.characters[i].sitting == SitState::Floor)
                    {
                        winners.push_back(s.map.characters[i]);
                    }
                }
            }

            if(winners.size() > 0)
            {
                unsigned int index = s.rand_gen.RandInt(0, winners.size() - 1);
                index += s.rand_gen.RandInt(0, 3);
                if(index >= winners.size()) index = s.rand_gen.RandInt(0, winners.size() - 1);

                Character winner = winners[index];
                this->winner = winner.gameworld_id;

                std::string name_upper = winner.name;
                name_upper[0] = std::toupper(winner.name[0]);

                std::string item_name = s.eif->Get(this->item_id).name;
                std::string item_amount_str = std::to_string(this->item_amount);

                std::string message = "Congratulations " + name_upper;
                message += ", you won " + item_name + " x" + item_amount_str + ".";
                s.eprocessor.DelayedMessage(message, 1000);
                message = "Please trade me to receive your award. (Available 30 seconds)";
                s.eprocessor.DelayedMessage(message, 5000);

                s.eoclient.TradeRequest(this->winner);
                s.eprocessor.sitwin.clock.restart();
            }
            else
            {
                s.eoclient.TalkPublic("Sorry, no one won.");
                this->run = false;

            }

            S::GetInstance().eoclient.Sit(SitAction::Stand);
        }
        else
        {
            int elapsed = this->clock.getElapsedTime().asSeconds();
            if(elapsed >= 30)
            {
                if(this->winner == -1)
                {
                    if(s.eprocessor.trade.get())
                    {
                        s.eoclient.TradeClose();
                    }

                    if(this->item_id != 0 && this->item_amount != 0)
                    {
                        this->Play();
                    }
                    else
                    {
                        this->gameworld_id = -1;
                        this->run = false;
                        s.eoclient.TalkPublic("Game canceled: no item selected.");
                        S::GetInstance().eoclient.Sit(SitAction::Stand);
                    }
                }
                else
                {
                    if(s.eprocessor.trade.get())
                    {
                        s.eoclient.TradeClose();
                    }

                    s.eoclient.TalkPublic("Time's up. The game has been finished.");
                    this->gameworld_id = -1;
                    this->run = false;
                    S::GetInstance().eoclient.Sit(SitAction::Stand);
                }
            }
        }
    }
}

void SitWin::Play()
{
    this->play = true;
    this->winner = -1;
    this->clock.restart();
}
