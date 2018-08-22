unsigned recursion(unsigned i)
{
    if (i > 0)
    {
        return recursion(i - 1);
    }
    return i;
}

int function(int x, int y)
{
    int i;
    int j = x;
    int k = y;
    recursion(5);
    return i + k;
}

int ifTest(int z)
{
    int result;
    if (z < 5)
    {
        result = 2;
    }
    else
    {
        result = -5;
    }
    int counter = 0;
    int calc = 0;
    while (result > 0)
    {
        calc += counter * result;
        counter = counter + 1;
    }
    return calc;
}

int main(int argc, char **argv)
{
    int i;
    int j;
    int k;
    k = function(j, 12);
    int f = 2;
    f = ifTest(2);
    j = 2;
    return 0;
}
