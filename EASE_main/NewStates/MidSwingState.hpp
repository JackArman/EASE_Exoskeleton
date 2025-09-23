#include "LegData.hpp"
#include "GeneralState.hpp"
#include "TerminalSwingState.hpp"
int i = 0;

float hipLeftGait[] = {};

float hipRightGait[] = {};

float kneeLeftGait[] = {};

float kneeRightGait[] = {};

int maxDataPoints = max(len(hipLeftGait), len(hipRightGait), len(kneeLeftGait), len(kneeRightGait));


class MidSwingState : public GeneralState {
    bool checkTransitionState() override {
      return false;
    }
    LegData getLegData() override {
      i++;
      if (i == maxDataPoints) {
        // Currently keep feeding last data point if we exhaust points to use
        i = maxDataPoints - 1;
      }
      return LegData{kneeLeftGait[i], kneeRightGait[i], hipLeftGait[i], hipRightGait[i]}
    }

    void transitionState() override {
      mc->changeState(new TerminalSwingState(mc));
    }
}