int sqar(int baz){
    return baz * baz;
}

int mul(int baz){
    return 100 * baz;
}
int foo(int bar, int x){
    int baz = x;
    int lbaz = mul(sqar(baz));
    return lbaz*bar;
}
 
int main(){
    int x = 5;
    int i = foo(13,x);
    int y = 0;
    int z = 0;
    return 0;
}