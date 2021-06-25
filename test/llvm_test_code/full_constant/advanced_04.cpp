int sqar(int baz){
    return baz * baz;
}
int foo(int bar, int x){
    int baz = x;
    int lbaz = sqar(baz);
    return lbaz*bar;
}
 
int main(){
    int x = 5;
    int i = foo(13,x);
    return 0;
}