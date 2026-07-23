#include <exception>
#include <iostream>

void runEliminationRuleTests();
void runBoardTests();
void runHeroTests();
void runProtocolTests();
void runViewTransformTests();
void runGuiClientTests();
void runCombatTests();
void runOpeningTurnTests();

int main() {
    try {
        runEliminationRuleTests();
        runBoardTests();
        runHeroTests();
        runProtocolTests();
        runViewTransformTests();
        runGuiClientTests();
        runCombatTests();
        runOpeningTurnTests();
        std::cout << "All tests passed.\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
