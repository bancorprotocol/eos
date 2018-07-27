#include <exchange/exchange_state.hpp>

namespace eosio {
   extended_asset exchange_state::convert_to_exchange( connector& c, extended_asset in ) {

      real_type R(supply.amount);
      real_type C(c.balance.amount+in.amount);
      real_type F(c.weight/1000.0);
      real_type T(in.amount);
      real_type ONE(1.0);

      real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
      int64_t issued = int64_t(E);

      supply.amount += issued;
      c.balance.amount += in.amount;

      return extended_asset( issued, supply.get_extended_symbol() );
   }

   extended_asset exchange_state::convert_from_exchange( connector& c, extended_asset in ) {
      eosio_assert( in.contract == supply.contract, "unexpected asset contract input" );
      eosio_assert( in.symbol== supply.symbol, "unexpected asset symbol input" );

      real_type R(supply.amount - in.amount);
      real_type C(c.balance.amount);
      real_type F(1000.0/c.weight);
      real_type E(in.amount);
      real_type ONE(1.0);


      real_type T = C * (std::pow( ONE + E/R, F) - ONE);
      int64_t out = int64_t(T);

      supply.amount -= in.amount;
      c.balance.amount -= out;

      return extended_asset( out, c.balance.get_extended_symbol() );
   }
   extended_asset exchange_state::convert_quick_inv(extended_symbol from, extended_asset to){
      auto sell_symbol  = from;
      auto to_symbol  = to.get_extended_symbol();
      auto base_symbol  = base.balance.get_extended_symbol();
      auto quote_symbol = quote.balance.get_extended_symbol();
      
      real_type balance_from = 0;
      real_type balance_to = 0;
      real_type amount_desired = to.amount;
      if(sell_symbol == base_symbol && to_symbol == quote_symbol){
         balance_from = base.balance.amount;
         balance_to   = quote.balance.amount;
      }
      else if(sell_symbol == quote_symbol && to_symbol == base_symbol){
         balance_from = quote.balance.amount;
         balance_to   = base.balance.amount;
      } else 
	  eosio_assert( false, "invalid conversion" );
      return extended_asset(int64_t(balance_from / (1.0 - amount_desired / balance_to) - balance_from), sell_symbol); 
   }
   extended_asset exchange_state::convert_quick(extended_asset from, extended_symbol to){
      auto sell_symbol  = from.get_extended_symbol();
      auto to_symbol  = to;
      auto base_symbol  = base.balance.get_extended_symbol();
      auto quote_symbol = quote.balance.get_extended_symbol();
      
      real_type amount_from = from.amount;
      real_type balance_from = 0;
      real_type balance_to = 0;
      if(sell_symbol == base_symbol && to_symbol == quote_symbol){
         balance_from = base.balance.amount;
         balance_to   = quote.balance.amount;
      }
      else if(sell_symbol == quote_symbol && to_symbol == base_symbol){
         balance_from = quote.balance.amount;
         balance_to   = base.balance.amount;
      }
      else eosio_assert( false, "invalid conversion" );
      return extended_asset(int64_t(amount_from / (balance_from + amount_from) * balance_to), to_symbol);
   }
   extended_asset exchange_state::convert( extended_asset from, extended_symbol to ) {
      auto sell_symbol  = from.get_extended_symbol();
      auto ex_symbol    = supply.get_extended_symbol();
      auto base_symbol  = base.balance.get_extended_symbol();
      auto quote_symbol = quote.balance.get_extended_symbol();

      if( sell_symbol != ex_symbol ) {
         if( sell_symbol == base_symbol ) {
            from = convert_to_exchange( base, from );
         } else if( sell_symbol == quote_symbol ) {
            from = convert_to_exchange( quote, from );
         } else { 
            eosio_assert( false, "invalid sell" );
         }
      } else {
         if( to == base_symbol ) {
            from = convert_from_exchange( base, from ); 
         } else if( to == quote_symbol ) {
            from = convert_from_exchange( quote, from ); 
         } else {
            eosio_assert( false, "invalid conversion" );
         }
      }

      if( to != from.get_extended_symbol() )
         return convert( from, to );

      return from;
   }

   bool exchange_state::requires_margin_call(  const exchange_state::connector& con )const {
      if( con.peer_margin.total_lent.amount > 0  ) {
         auto tmp = *this;
         auto base_total_col = int64_t(con.peer_margin.total_lent.amount * con.peer_margin.least_collateralized);
         auto covered = tmp.convert( extended_asset( base_total_col, con.balance.get_extended_symbol()), con.peer_margin.total_lent.get_extended_symbol() );
         if( covered.amount <= con.peer_margin.total_lent.amount ) 
            return true;
      }
      return false;
   }

   bool exchange_state::requires_margin_call()const {
      return requires_margin_call( base ) || requires_margin_call( quote );
   }


} /// namespace eosio
