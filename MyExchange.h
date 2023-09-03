#pragma once

#include "IExchange.h"

#include <list>
#include <map>
#include <set>

using Symbol = std::string;

class MyExchange : public IExchange
{
  public:
    MyExchange();

    virtual void InsertOrder(
        const std::string& symbol, Side side, Price price, Volume volume, UserReference userReference) override;
    virtual void DeleteOrder(OrderId orderId) override;

  private:
    struct OrderInfo;
    struct PriceLevel;
    struct OrderBook;

    using OrderIdToInfoMap = std::map<OrderId, OrderInfo>;
    using OrderBookMap     = std::map<Symbol, OrderBook>;
    using OrderIterList    = std::list<OrderIdToInfoMap::iterator>;

    struct OrderInfo
    {
        Symbol        symbol;
        Side          side;
        Price         price;
        Volume        vol;
        UserReference userReference;

        // pos of order book for this order
        OrderBookMap::iterator book_pos;
        // pos of price level in order book for this order
        std::map<Price, PriceLevel>::iterator price_pos;
        // position of order in OrderList of price level of this order
        OrderIterList::iterator order_pos;

        OrderInfo(Symbol symbol, Side side, Price price, Volume vol, UserReference userReference)
            : symbol(symbol), side(side), price(price), vol(vol), userReference(userReference)
        {
        }
    };

    struct PriceLevel
    {
        // Total volume at current price level
        Volume total_vol{0};
        // List containing the iterators to order info
        OrderIterList order_list;
    };

    struct OrderBook
    {
        // Ask Price levels
        std::map<Price, PriceLevel> ask_price_level;
        // Bid Price Levels
        std::map<Price, PriceLevel, std::greater<Price>> bid_price_level;

        // Best bid price and total volume for that price level
        Price  best_bid_price{0};
        Volume best_bid_total_vol{0};

        // Best ask price and total volume for that price level
        Price  best_ask_price{0};
        Volume best_ask_total_vol{0};
    };

    // OrderId to Order Info map
    OrderIdToInfoMap m_orderid_to_info;

    // Symbol to OrderBook Map
    std::map<Symbol, OrderBook> m_order_book;

    // Set of supported symbols ( currently filled in constructor)
    std::set<Symbol> m_symbol_list;

    // Next Order id, this keeps track of Unique incremental order ids to be given
    // to new orders
    int m_next_order_id;
};
