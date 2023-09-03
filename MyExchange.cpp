#include "MyExchange.h"
#include <iostream>

MyExchange::MyExchange() : m_next_order_id(1)
{
    m_symbol_list = {"AAPL", "MSFT", "GOOG"};
}

void MyExchange::InsertOrder(
    const std::string& symbol, Side side, Price price, Volume volume, UserReference userReference)
{
    if (m_symbol_list.count(symbol) == 0)
    {
        if (IExchange::OnOrderInserted)
        {
            IExchange::OnOrderInserted(userReference, InsertError::SymbolNotFound, 0);
        }
        return;
    }

    if (price <= 0)
    {
        if (IExchange::OnOrderInserted)
        {
            IExchange::OnOrderInserted(userReference, InsertError::InvalidPrice, 0);
        }
        return;
    }

    if (volume <= 0)
    {
        if (IExchange::OnOrderInserted)
        {
            IExchange::OnOrderInserted(userReference, InsertError::InvalidVolume, 0);
        }
        return;
    }

    OrderId order_id = m_next_order_id++;
    // Create OrderInfo with new order_id
    auto[orderinfo_pos, temp1]
        = m_orderid_to_info.emplace(order_id, OrderInfo(symbol, side, price, volume, userReference));
    OrderInfo& order = orderinfo_pos->second;

    // Create new order book for symbol if already not present
    auto[book_pos, temp2] = m_order_book.emplace(symbol, OrderBook());
    OrderBook& order_book = book_pos->second;

    bool isBestPriceChanged = false;

    if (side == Side::Sell)
    {
        // create price level for this order if not already present
        auto[price_pos, temp3] = order_book.ask_price_level.emplace(price, PriceLevel());
        PriceLevel& price_level = price_pos->second;
        price_level.total_vol += volume;

        // store iterators for order, price_level and book
        order.order_pos = price_level.order_list.insert(price_level.order_list.end(), orderinfo_pos);
        order.book_pos  = book_pos;
        order.price_pos = price_pos;

        // if price level is top level in order book then update best price 
        if (price_pos == order_book.ask_price_level.begin())
        {
            isBestPriceChanged = true;
            order_book.best_ask_price = price;
            order_book.best_ask_total_vol = price_level.total_vol;
        }
    }
    else
    {
        // create price level for this order if not already present
        auto[price_pos, temp3] = order_book.bid_price_level.emplace(price, PriceLevel());
        PriceLevel& price_level = price_pos->second;
        // increase total volume at that price level
        price_level.total_vol += volume;

        // store iterators for order, price_level and book
        order.order_pos = price_level.order_list.insert(price_level.order_list.end(), orderinfo_pos);
        order.book_pos  = book_pos;
        order.price_pos = price_pos;

        // if price level is top level in order book then update best price 
        if (price_pos == order_book.bid_price_level.begin())
        {
            isBestPriceChanged = true;
            order_book.best_bid_price = price;
            order_book.best_bid_total_vol = price_level.total_vol;
        }
    }

    if (IExchange::OnOrderInserted)
    {
        IExchange::OnOrderInserted(userReference, InsertError::OK, order_id);
    }

    if (isBestPriceChanged && IExchange::OnBestPriceChanged)
    {
        IExchange::OnBestPriceChanged(symbol,
                                      order_book.best_bid_price,
                                      order_book.best_bid_total_vol,
                                      order_book.best_ask_price,
                                      order_book.best_ask_total_vol);
    }

    return;
}

void MyExchange::DeleteOrder(OrderId orderId)
{
    // Find oder in order_id to order_info map 
    auto orderinfo_pos = m_orderid_to_info.find(orderId);
    if (orderinfo_pos == m_orderid_to_info.end())
    {
        // If order not found then return with error
        if (IExchange::OnOrderDeleted)
        {
            IExchange::OnOrderDeleted(orderId, DeleteError::OrderNotFound);
        }
        return;
    }

    // get all iterators from order_info for order_book, price_level and order
    OrderInfo&  order       = orderinfo_pos->second;
    Symbol      symbol      = order.symbol;
    PriceLevel& price_level = order.price_pos->second;
    OrderBook&  order_book  = order.book_pos->second;

    // Erase order from list at the same price level
    price_level.order_list.erase(order.order_pos);
    // decrease total volume for that price level
    price_level.total_vol -= order.vol;

    auto price_pos = order.price_pos;

    bool isBestPriceChanged = false;

    if (order.side == Side::Sell)
    {
        // If current price level is empty then delete this level, and make the next level as current level
        if (price_level.total_vol == 0)
        {
            price_pos = order_book.ask_price_level.erase(price_pos);
        }
        // if the current price_level after deletion is the top of the order book then update the best price
        if (price_pos == order_book.ask_price_level.begin())
        {
            isBestPriceChanged = true;
            // if all price levels are empty then reset the best price
            if (price_pos == order_book.ask_price_level.end())
            {
                order_book.best_ask_price = 0;
                order_book.best_ask_total_vol = 0;
            }
            else
            {
                order_book.best_ask_price = price_pos->first;
                order_book.best_ask_total_vol = price_pos->second.total_vol;
            }
        }
    }
    else
    {
        // If current price level is empty then delete this level, and make the next level as current level
        if (price_level.total_vol == 0)
        {
            price_pos = order_book.bid_price_level.erase(price_pos);
        }
        // if the current price_level after deletion is the top of the order book then update the best price
        if (price_pos == order_book.bid_price_level.begin())
        {
            isBestPriceChanged = true;
            // if all price levels are empty then reset the best price
            if (price_pos == order_book.bid_price_level.end())
            {
                order_book.best_bid_price = 0;
                order_book.best_bid_total_vol = 0;
            }
            else
            {
                order_book.best_bid_price = price_pos->first;
                order_book.best_bid_total_vol = price_pos->second.total_vol;
            }
        }
    }

    // erasing order from OrderInfo map
    m_orderid_to_info.erase(orderinfo_pos);

    if (IExchange::OnOrderDeleted)
    {
        IExchange::OnOrderDeleted(orderId, DeleteError::OK);
    }

    if (isBestPriceChanged && IExchange::OnBestPriceChanged)
    {
        IExchange::OnBestPriceChanged(symbol,
                                      order_book.best_bid_price,
                                      order_book.best_bid_total_vol,
                                      order_book.best_ask_price,
                                      order_book.best_ask_total_vol);
    }

    return;
}
