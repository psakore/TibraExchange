#include "IExchange.h"
#define BOOST_TEST_MODULE YourExchange test
#include <boost/test/included/unit_test.hpp>
// Please use a meaningful name here, ie.
#include "MyExchange.h"
#include <tuple>
namespace Tibra {
namespace Exchange {
namespace Test {
/// This class sets up the necessary prerequisites for a unit test
class ExchangeFixtures
{
  public:
    ExchangeFixtures();
    void OrderInsertedHandler(UserReference userReference, InsertError insertError, OrderId orderId)
    {
        mOrderInsertedEvents.emplace_back(userReference, insertError, orderId);
    }

    void OrderDeletedHandler(OrderId orderId, DeleteError deleteError)
    {
        mOrderDeletedEvents.emplace_back(orderId, deleteError);
    }

    void BestPriceChangedHandler(
        const std::string& symbol, Price bestBid, Volume totalBidVolume, Price bestAsk, Volume totalAskVolume)
    {
        mBestPriceChangedEvents.emplace_back(symbol, bestBid, totalBidVolume, bestAsk, totalAskVolume);
    }

    using OrderInsertedEvent  = std::tuple<UserReference, InsertError, OrderId>;
    using OrderInsertedEvents = std::vector<OrderInsertedEvent>;
    OrderInsertedEvents mOrderInsertedEvents;

    using OrderDeletedEvent  = std::tuple<OrderId, DeleteError>;
    using OrderDeletedEvents = std::vector<OrderDeletedEvent>;
    OrderDeletedEvents mOrderDeletedEvents;

    using BestPriceChangedEvent  = std::tuple<std::string, Price, Volume, Price, Volume>;
    using BestPriceChangedEvents = std::vector<BestPriceChangedEvent>;
    BestPriceChangedEvents mBestPriceChangedEvents;

    // This is the code under test
    MyExchange mExchange;
};
ExchangeFixtures::ExchangeFixtures()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    // OrderInserted events are logged to a std::vector for later checking
    mExchange.OnOrderInserted = std::bind(&ExchangeFixtures::OrderInsertedHandler, this, _1, _2, _3);

    mExchange.OnOrderDeleted = std::bind(&ExchangeFixtures::OrderDeletedHandler, this, _1, _2);

    mExchange.OnBestPriceChanged = std::bind(&ExchangeFixtures::BestPriceChangedHandler, this, _1, _2, _3, _4, _5);
}

BOOST_FIXTURE_TEST_SUITE(ExchangeTests, ExchangeFixtures)

BOOST_AUTO_TEST_CASE(TestInsertInvalidSymbol)
{
    const UserReference clientOrderId = 2;
    mExchange.InsertOrder("INVALID", Side::Buy, 100, 10, clientOrderId);

    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderInsertedEvents.front()), clientOrderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderInsertedEvents.front()), InsertError::SymbolNotFound);
}

BOOST_AUTO_TEST_CASE(TestInsertInvalidPrice)
{
    const UserReference clientOrderId = 3;
    mExchange.InsertOrder("AAPL", Side::Buy, 0, 10, clientOrderId);

    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderInsertedEvents.front()), clientOrderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderInsertedEvents.front()), InsertError::InvalidPrice);
}

BOOST_AUTO_TEST_CASE(TestInsertInvalidVolume)
{
    const UserReference clientOrderId = 4;
    mExchange.InsertOrder("AAPL", Side::Buy, 100, 0, clientOrderId);

    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderInsertedEvents.front()), clientOrderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderInsertedEvents.front()), InsertError::InvalidVolume);
}

BOOST_AUTO_TEST_CASE(TestInvalidOrderId)
{
    OrderId orderId = 999;
    mExchange.DeleteOrder(orderId);
    // Must have fired the OnOrderInserted event
    BOOST_REQUIRE(!mOrderDeletedEvents.empty());
    // Must match our OrderId:
    BOOST_CHECK_EQUAL(std::get<0>(mOrderDeletedEvents.back()), orderId);
    // Must have the error code set:
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents.back()), DeleteError::OrderNotFound);
}

BOOST_AUTO_TEST_CASE(TestInsertValidOrder)
{
    const UserReference clientOrderId = 1;
    mExchange.InsertOrder("AAPL", Side::Buy, 100, 10, clientOrderId);

    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderInsertedEvents.front()), clientOrderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderInsertedEvents.front()), InsertError::OK);
}

BOOST_AUTO_TEST_CASE(TestInsertValidOrderi_GOOG)
{
    const UserReference clientOrderId = 1;
    mExchange.InsertOrder("GOOG", Side::Buy, 100, 10, clientOrderId);

    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderInsertedEvents.front()), clientOrderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderInsertedEvents.front()), InsertError::OK);
}

BOOST_AUTO_TEST_CASE(TestDeleteValidOrder)
{
    const UserReference clientOrderId = 5;
    mExchange.InsertOrder("AAPL", Side::Buy, 100, 10, clientOrderId);
    BOOST_REQUIRE(!mOrderInsertedEvents.empty());
    const OrderId orderId = std::get<2>(mOrderInsertedEvents.front());
    mExchange.DeleteOrder(orderId);

    BOOST_REQUIRE(!mOrderDeletedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mOrderDeletedEvents.front()), orderId);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents.front()), DeleteError::OK);
}

BOOST_AUTO_TEST_CASE(TestBestPriceChanged)
{
    const UserReference clientOrderId1 = 6;
    const UserReference clientOrderId2 = 7;

    // Insert buy order with higher price
    mExchange.InsertOrder("AAPL", Side::Buy, 100, 10, clientOrderId1);
    BOOST_REQUIRE(!mBestPriceChangedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents.front()), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents.front()), 100);
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents.front()), 10);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents.front()), 0);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents.front()), 0);
    mBestPriceChangedEvents.clear();

    // Insert sell order with lower price
    mExchange.InsertOrder("AAPL", Side::Sell, 90, 5, clientOrderId2);
    BOOST_REQUIRE(!mBestPriceChangedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents.front()), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents.front()), 100);
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents.front()), 10);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents.front()), 90);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents.front()), 5);
    mBestPriceChangedEvents.clear();

    // Insert sell order with higher price (shouldn't change best prices)
    mExchange.InsertOrder("AAPL", Side::Sell, 95, 8, clientOrderId2);
    BOOST_REQUIRE(mBestPriceChangedEvents.empty());

    // Delete buy order (should update best bid)
    const OrderId buyOrderId = std::get<2>(mOrderInsertedEvents[1]);
    mExchange.DeleteOrder(buyOrderId);
    BOOST_REQUIRE(!mBestPriceChangedEvents.empty());
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents.front()), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents.front()), 100);
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents.front()), 10);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents.front()), 95);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents.front()), 8);
    mBestPriceChangedEvents.clear();
}

BOOST_AUTO_TEST_CASE(TestComplexScenario)
{
    // Insert initial orders
    const UserReference clientOrderId1 = 1;
    const UserReference clientOrderId2 = 2;
    const UserReference clientOrderId3 = 3;

    mExchange.InsertOrder("AAPL", Side::Buy, 100, 10, clientOrderId1);
    mExchange.InsertOrder("AAPL", Side::Sell, 110, 8, clientOrderId2);
    mExchange.InsertOrder("AAPL", Side::Buy, 120, 15, clientOrderId3);

    BOOST_REQUIRE_EQUAL(mOrderInsertedEvents.size(), 3);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 3);

    // Check best prices after initial insertions
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[2]), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[2]), 120);  // AAPL best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[2]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[2]), 110);  // AAPL best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[2]), 8);
    mBestPriceChangedEvents.clear();

    // Delete an order
    const OrderId orderIdToDelete = std::get<2>(mOrderInsertedEvents.front());
    mExchange.DeleteOrder(orderIdToDelete);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 0);

    mOrderInsertedEvents.clear();
    // Insert more orders
    const UserReference clientOrderId4 = 4;
    const UserReference clientOrderId5 = 5;

    mExchange.InsertOrder("AAPL", Side::Buy, 125, 12, clientOrderId4);
    mExchange.InsertOrder("AAPL", Side::Sell, 110, 10, clientOrderId5);

    BOOST_REQUIRE_EQUAL(mOrderInsertedEvents.size(), 2);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 2);

    // Check best prices after additional insertions
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 125);  // AAPL best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 12);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 110);  // AAPL best ask (unchanged)
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 8);

    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[1]), "AAPL");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[1]), 125);  // AAPL best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[1]), 12);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[1]), 110);  // AAPL best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[1]), 18);

    mOrderDeletedEvents.clear();
    mBestPriceChangedEvents.clear();
    // Delete more orders
    const OrderId orderIdToDelete2 = std::get<2>(mOrderInsertedEvents[1]);

    mExchange.DeleteOrder(orderIdToDelete2);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 1);
}

BOOST_AUTO_TEST_CASE(TestBuyMultipleOrdersSamePriceLevel)
{
    // Insert initial orders
    const UserReference clientOrderId1 = 1;
    const UserReference clientOrderId2 = 2;
    const UserReference clientOrderId3 = 3;

    mExchange.InsertOrder("MSFT", Side::Buy, 100, 10, clientOrderId1);
    mExchange.InsertOrder("MSFT", Side::Buy, 100, 5, clientOrderId2);
    mExchange.InsertOrder("MSFT", Side::Buy, 100, 7, clientOrderId3);

    BOOST_REQUIRE_EQUAL(mOrderInsertedEvents.size(), 3);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 3);

    // Check best prices and total volumes after initial insertions
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 100);  // MSFT best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 10);   // Total volume
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 0);    // No ask side
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 0);

    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[1]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[1]), 100);  // MSFT best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[1]), 15);   // Total volume (unchanged)
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[1]), 0);    // No ask side (unchanged)
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[1]), 0);

    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[2]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[2]), 100);  // MSFT best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[2]), 22);   // Total volume (unchanged)
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[2]), 0);    // No ask side (unchanged)
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[2]), 0);

    mBestPriceChangedEvents.clear();
    // Delete the second buy order
    const OrderId orderIdToDelete = std::get<2>(mOrderInsertedEvents[1]);
    mExchange.DeleteOrder(orderIdToDelete);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 1);

    // Check best prices and total volumes after deletion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 100);  // MSFT best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 17);   // Updated total volume
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 0);    // No ask side
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 0);

    mBestPriceChangedEvents.clear();
    mOrderDeletedEvents.clear();
    // Delete the third buy order
    const OrderId orderIdToDelete2 = std::get<2>(mOrderInsertedEvents[2]);
    mExchange.DeleteOrder(orderIdToDelete2);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 1);

    // Check best prices and total volumes after deletion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 100);  // MSFT best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 10);   // Updated total volume
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 0);    // No ask side
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 0);

    mBestPriceChangedEvents.clear();
    mOrderDeletedEvents.clear();
    // Delete the second buy order again which should fail
    mExchange.DeleteOrder(orderIdToDelete2);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 0);
    BOOST_CHECK_EQUAL(std::get<0>(mOrderDeletedEvents[0]), orderIdToDelete2);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[0]),
                      DeleteError::OrderNotFound);  // OrderId not found as already deleted


    mBestPriceChangedEvents.clear();
    mOrderDeletedEvents.clear();
    // Delete the third buy order
    const OrderId orderIdToDelete3 = std::get<2>(mOrderInsertedEvents[0]);
    mExchange.DeleteOrder(orderIdToDelete3);

    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 1);

    // Check best prices and total volumes after deletion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 0);  // Updated best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 0);  // Updated total volume
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 0);  // No ask side
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 0);
}

BOOST_AUTO_TEST_CASE(TestComplexMultipleOrdersSamePriceLevel)
{
    // Buy 10 units at price 100
    const UserReference clientOrderId1 = 1;
    mExchange.InsertOrder("MSFT", Side::Buy, 100, 10, clientOrderId1);

    // Buy 15 units at price 110
    const UserReference clientOrderId2 = 2;
    mExchange.InsertOrder("MSFT", Side::Buy, 110, 15, clientOrderId2);

    // Sell 10 units at price 90
    const UserReference clientOrderId3 = 3;
    mExchange.InsertOrder("MSFT", Side::Sell, 90, 10, clientOrderId3);

    // Sell 10 units at price 100
    const UserReference clientOrderId4 = 4;
    mExchange.InsertOrder("MSFT", Side::Sell, 100, 10, clientOrderId4);

    // Buy 15 units at price 100
    const UserReference clientOrderId5 = 5;
    mExchange.InsertOrder("MSFT", Side::Buy, 100, 15, clientOrderId5);

    // Sell 5 units at price 90
    const UserReference clientOrderId6 = 6;
    mExchange.InsertOrder("MSFT", Side::Sell, 90, 5, clientOrderId6);

    BOOST_REQUIRE_EQUAL(mOrderInsertedEvents.size(), 6);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 4);

    // Check best prices and total volumes after each insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[0]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[0]), 100);  // MSFT best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[0]), 10);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[0]), 0);  // No best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[0]), 0);

    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[1]), 110);  // MSFT best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[1]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[1]), 0);  // No best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[1]), 0);

    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[2]), 110);  // MSFT best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[2]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[2]), 90);  // MSFT best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[2]), 10);

    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[3]), 110);  // MSFT best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[3]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[3]), 90);  // MSFT best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[3]), 15);

    // Sell 5 more units at price 90
    const UserReference clientOrderId7 = 7;
    mExchange.InsertOrder("MSFT", Side::Sell, 90, 5, clientOrderId7);

    BOOST_REQUIRE_EQUAL(mOrderInsertedEvents.size(), 7);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 5);

    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[4]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[4]), 110);  // MSFT best bid (unchanged)
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[4]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[4]), 90);  // Updated best ask
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[4]), 20);  // Updated total volume

    const OrderId orderId1 = std::get<2>(mOrderInsertedEvents[0]);
    const OrderId orderId2 = std::get<2>(mOrderInsertedEvents[1]);
    const OrderId orderId3 = std::get<2>(mOrderInsertedEvents[2]);
    const OrderId orderId4 = std::get<2>(mOrderInsertedEvents[3]);
    const OrderId orderId5 = std::get<2>(mOrderInsertedEvents[4]);
    const OrderId orderId6 = std::get<2>(mOrderInsertedEvents[5]);
    const OrderId orderId7 = std::get<2>(mOrderInsertedEvents[6]);

    mExchange.DeleteOrder(orderId1);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 5);  // no change

    mExchange.DeleteOrder(orderId2);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 6);
    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[5]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[5]), 100);  // Updated best bid
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[5]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[5]), 90);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[5]), 20);

    mExchange.DeleteOrder(orderId3);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 7);
    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[6]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[6]), 100);
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[6]), 15);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[6]), 90);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[6]), 10);  // Updated total volume

    mExchange.DeleteOrder(orderId5);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 8);
    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[7]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[7]), 0);  // no bid left
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[7]), 0);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[7]), 90);
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[7]), 10);

    mExchange.DeleteOrder(orderId6);
    mExchange.DeleteOrder(orderId7);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 10);
    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[9]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[9]), 0);  // no bid left
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[9]), 0);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[9]), 100);  // price level changed
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[9]), 10);

    mExchange.DeleteOrder(orderId4);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 11);
    // Check best prices and total volumes after the additional insertion
    BOOST_CHECK_EQUAL(std::get<0>(mBestPriceChangedEvents[10]), "MSFT");
    BOOST_CHECK_EQUAL(std::get<1>(mBestPriceChangedEvents[10]), 0);  // no bid left
    BOOST_CHECK_EQUAL(std::get<2>(mBestPriceChangedEvents[10]), 0);
    BOOST_CHECK_EQUAL(std::get<3>(mBestPriceChangedEvents[10]), 0);  // no ask left
    BOOST_CHECK_EQUAL(std::get<4>(mBestPriceChangedEvents[10]), 0);


    mExchange.DeleteOrder(orderId1);
    mExchange.DeleteOrder(orderId2);
    mExchange.DeleteOrder(orderId3);
    BOOST_REQUIRE_EQUAL(mOrderDeletedEvents.size(), 10);
    BOOST_REQUIRE_EQUAL(mBestPriceChangedEvents.size(), 11);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[0]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[1]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[2]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[3]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[4]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[5]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[6]), DeleteError::OK);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[7]), DeleteError::OrderNotFound);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[8]), DeleteError::OrderNotFound);
    BOOST_CHECK_EQUAL(std::get<1>(mOrderDeletedEvents[9]), DeleteError::OrderNotFound);
}


BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test
}  // namespace Exchange
}  // namespace Tibra
