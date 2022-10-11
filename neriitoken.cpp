#include "neriitoken.hpp"

namespace inery {

void nerii::create( const name&   issuer,
                    const asset&  maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "NGETIK YANG BENER MAS" );
    check( maximum_supply.is_valid(), "NGETIK YANG BENER!!!");
    check( maximum_supply.amount > 0, "MAX SUPPLY NGGA BOLEH MINES");

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "NGETIK YANG BENER!!!" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void nerii::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "NGETIK YANG BENER!!!" );
    check( memo.size() <= 256, "JANGAN PANJANG-PANJANG MEMO NYA" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "TOKEN DIBIKIN AJA BELOM" );
    const auto& st = *existing;
    check( to == st.issuer, "INI BUKAN CONTRACT MU" );

    require_auth( st.issuer );
    check( quantity.is_valid(), "YANG BENER DIKIT" );
    check( quantity.amount > 0, "NGETIK YANG BENER!!!" );

    check( quantity.symbol == st.supply.symbol, "NGETIK YANG BENER!!!" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "OVERLOAD");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}

void nerii::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "NGETIK YANG BENER!!!" );
    check( memo.size() <= 256, "JANGAN PANJANG-PANJANG MEMO NYA" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "NGGA ADA TOKENNYA!" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "SALAH JUMLAH" );
    check( quantity.amount > 0, "NGGA BOLEH MINES" );

    check( quantity.symbol == st.supply.symbol, "SALAH SALAH" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void nerii::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "TRANSFER KE ORANG LAIN LA" );
    require_auth( from );
    check( is_account( to ), "NGIRIM KE SIAPA, NGGA ADA NAMA GITUAN");
    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "SALAH JUMLAH" );
    check( quantity.amount > 0, "NGGA BOLEH MINES" );
    check( quantity.symbol == st.supply.symbol, "SALAH SALAH" );
    check( memo.size() <= 256, "JANGAN PANJANG-PANJANG MEMO NYA" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void nerii::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void nerii::add_balance( const name& owner, const asset& value, const name& mem_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( mem_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void nerii::open( const name& owner, const symbol& symbol, const name& mem_payer )
{
   require_auth( mem_payer );

   check( is_account( owner ), "owner account does not exist" );

   auto sym_code_raw = symbol.code().raw();
   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( mem_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void nerii::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

} /// namespace inery
