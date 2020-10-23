class Parent {
public:
  Parent() = default;
  virtual ~Parent() = default;
};

class PropertyKeyElement : public Parent {
private:
  int x;

public:
  PropertyKeyElement() = default;
  virtual ~PropertyKeyElement() = default;

  virtual void foo(){};
  virtual void bar(){};
  virtual void baz(){};
  virtual void bant(){};
};

class AElement : public Parent {
private:
  int x;

public:
  AElement() = default;
  virtual ~AElement() = default;
};

class NEDTypeInfo {
public:
  void mergePropertyKey(PropertyKeyElement *basekey,
                        const PropertyKeyElement *key) const;

public:
  virtual ~NEDTypeInfo() = default;
  virtual void package() const;
};

void NEDTypeInfo::package() const { AElement *a = nullptr; }

void NEDTypeInfo::mergePropertyKey(PropertyKeyElement *basekey,
                                   const PropertyKeyElement *key) const {
  basekey->bant();
}

int main(int argc, char **argv) {
  PropertyKeyElement k;
  NEDTypeInfo n;
  n.mergePropertyKey(&k, &k);
  AElement f;
}
