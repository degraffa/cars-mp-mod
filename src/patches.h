#define P(va,o,n) {va,(const unsigned char*)o,(const unsigned char*)n,sizeof(o)-1}
static const Patch patches[]={
 /* race HUD name: 3+ players fall through to the 2 player overlay instead of NULL */
 P(0x46bb95,"\x33\xc0\xc3","\xeb\x01\x90"),
};
