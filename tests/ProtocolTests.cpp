#include "tests/TestHarness.hpp"
#include "tests/TestSupport.hpp"

#include "common/Protocol.hpp"

using namespace sts;

void runProtocolTests() {
    const EliminateRequest request{1, 9, {{4, 0}, {4, 1}, {3, 1}}};
    const auto decodedRequest = deserializeEliminateRequest(serialize(request));
    REQUIRE(decodedRequest.has_value());
    REQUIRE(decodedRequest->playerId == 1);
    REQUIRE(decodedRequest->turnId == 9);
    REQUIRE(decodedRequest->path == request.path);

    const auto frame = makeLengthPrefixedFrame("hello");
    REQUIRE(frame.size() == 9U);
    const std::array<std::byte, 4> header{frame[0], frame[1], frame[2], frame[3]};
    REQUIRE(decodeFrameLength(header).value() == 5U);

    const EliminateResult result{true, "", 0, 10, test::solidBoard().snapshot()};
    const auto decodedResult = deserializeEliminateResult(serialize(result));
    REQUIRE(decodedResult.has_value());
    REQUIRE(decodedResult->success);
    REQUIRE(decodedResult->nextPlayerId == 0);
    REQUIRE(decodedResult->nextTurnId == 10);

    const GameStartedMessage started{0, 1, result.board};
    const auto decodedStarted = deserializeGameStartedMessage(serialize(started));
    REQUIRE(decodedStarted.has_value());
    REQUIRE(decodedStarted->board == result.board);
}
