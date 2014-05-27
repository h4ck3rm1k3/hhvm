<?hh

///////////////////////////////////////////////////////////////////////////////
// helpers

// This doc comment block generated by idl/sysdoc.php
/**
 * ( excerpt from http://php.net/manual/en/class.reflector.php )
 *
 * Reflector is an interface implemented by all exportable Reflection
 * classes.
 *
 */
interface Reflector {
  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflector.tostring.php )
   *
   * To string. Warning: This function is currently not documented; only its
   * argument list is available.
   *
   */
  public function __toString();
}

// This doc comment block generated by idl/sysdoc.php
/**
 * ( excerpt from http://php.net/manual/en/class.reflectionexception.php )
 *
 * The ReflectionException class.
 *
 */
class ReflectionException extends Exception {
}

///////////////////////////////////////////////////////////////////////////////
// parameter

// This doc comment block generated by idl/sysdoc.php
/**
 * ( excerpt from http://php.net/manual/en/class.reflectionparameter.php )
 *
 * The ReflectionParameter class retrieves information about function's or
 * method's parameters.
 *
 * To introspect function parameters, first create an instance of the
 * ReflectionFunction or ReflectionMethod classes and then use their
 * ReflectionFunctionAbstract::getParameters() method to retrieve an array
 * of parameters.
 *
 */
class ReflectionParameter implements Reflector {
  public $info;
  public $name;

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.construct.php )
   *
   * Constructs a ReflectionParameter class. Warning: This function is
   * currently not documented; only its argument list is available.
   *
   * @func       mixed   The function to reflect parameters from.
   * @param      mixed   The parameter.
   *
   * @return     mixed   No value is returned.
   */
  public function __construct($func, $param) {
    if (is_null($func) && is_null($param)) {
      return;
    }

    if ($func instanceof Closure) {
      $params = (new ReflectionFunction($func))->getParameters();
    } else if (is_string($func)) {
      $double_colon = strpos($func, "::");
      if ($double_colon === false) {
        $params = (new ReflectionFunction($func))->getParameters();
      } else {
        $class = substr($func, 0, $double_colon);
        $method = substr($func, $double_colon + 2);
        $params = (new ReflectionMethod($class, $method))->getParameters();
      }
    } else if (is_array($func)) {
      $params = (new ReflectionMethod($func[0], $func[1]))->getParameters();
    } else {
      throw new Exception(
        "Invalid function, expected string, got ".gettype($func)
      );
    }

    if (is_string($param)) {
      foreach ($params as $p) {
        if ($p->name === $param) {
          $this->info = $p->info;
          $this->name = $p->name;
          break;
        }
      }
    } else if (is_int($param) && $param < count($params)) {
      $p = $params[$param];
      $this->info = $p->info;
      $this->name = $p->name;
    } else {
      throw new Exception("No param named $param found");
    }
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionparameter.tostring.php
   * )
   *
   * To string. Warning: This function is currently not documented; only its
   * argument list is available.
   *
   */
  public function __toString() {
    $type = $this->getTypeText();
    if ($type !== '') {
      if ($this->isOptional() && $this->getDefaultValue() === null) {
        $type .= ' or NULL';
      }
      $type .= ' ';
    }
    $out = 'Parameter #'.$this->getPosition().' [ ';
    if ($this->isOptional()) {
      $default = var_export($this->getDefaultValue(), true);
      $out .= '<optional> '.$type.'$'.$this->getName().' = '.$default;
    } else {
      $out .= '<required> '.$type.'$'.$this->getName();
    }
    $out .= ' ]';
    return $out;
  }

  // Prevent cloning
  public function __clone() {
    throw new BadMethodCallException(
      'Trying to clone an uncloneable object of class ReflectionParameter'
    );
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionparameter.export.php )
   *
   * Exports. Warning: This function is currently not documented; only its
   * argument list is available.
   *
   * @func       mixed   The function name.
   * @param      mixed   The parameter name.
   * @ret        mixed   Setting to TRUE will return the export, as opposed
   *                     to emitting it. Setting to FALSE (the default) will
   *                     do the opposite.
   *
   * @return     mixed   The exported reflection.
   */
  public static function export($func, $param, $ret=false) {
    $obj = new ReflectionParameter($func, $param);
    $str = (string)$obj;
    if ($ret) {
      return $str;
    }
    print $str;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionparameter.getname.php
   * )
   *
   * Gets the name of the parameter.
   *
   * @return     mixed   The name of the reflected parameter.
   */
  public function getName() {
    return $this->info['name'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.ispassedbyreference.php )
   *
   * Checks if the parameter is passed in by reference. Warning: This
   * function is currently not documented; only its argument list is
   * available.
   *
   * @return     mixed   TRUE if the parameter is passed in by reference,
   *                     otherwise FALSE
   */
  public function isPassedByReference() {
    return isset($this->info['ref']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/de/reflectionparameter.canbepassedbyvalue.php )
   *
   * Returns whether this parameter can be passed by value. Warning: This
   * function is currently not documented; only its argument list is
   * available.
   *
   * @return     mixed   Returns TRUE if the parameter can be passed by value,
                         FALSE otherwise. Returns NULL in case of an error.
   */
  public function canBePassedByValue() {
    return !isset($this->info['ref']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.getdeclaringclass.php )
   *
   * Gets the declaring class. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @return     mixed   A ReflectionClass object.
   */
  public function getDeclaringClass() {
    if (empty($this->info['class'])) {
      return null;
    }
    return new ReflectionClass($this->info['class']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.getdeclaringfunction.php )
   *
   * Gets the declaring function. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @return     mixed   A ReflectionFunction object.
   */
  public function getDeclaringFunction() {
    if (empty($this->info['class'])) {
      return new ReflectionFunction($this->info['function']);
    }
    return new ReflectionMethod($this->info['class'], $this->info['function']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionparameter.getclass.php
   * )
   *
   * Gets a class. Warning: This function is currently not documented; only
   * its argument list is available.
   *
   * @return     mixed   A ReflectionClass object.
   */
  public function getClass() {
    if (empty($this->info['type'])) {
      return null;
    }
    $ltype = strtolower($this->info['type']);
    if (hphp_scalar_typehints_enabled()) {
      $nonClassTypehints = array(
        'HH\bool' => 1,
        'HH\int' => 1,
        'HH\float' => 1,
        'HH\string' => 1,
        'array' => 1
      );
      if (isset($nonClassTypehints[$ltype])) {
        return null;
      }
    } else if ($ltype === 'array') {
      return null;
    }
    return new ReflectionClass($this->info['type']);
  }

  private static function stripHHPrefix($str) {
    if (!is_string($str)) return $str;
    return str_ireplace(
      array('HH\\bool', 'HH\\int', 'HH\\float', 'HH\\string', 'HH\\num',
            'HH\\resource', 'HH\\void', 'HH\\this'),
      array('bool',     'int',     'float',     'string',     'num',
            'resource',     'void', 'this'),
      $str
    );
  }

  public function getTypehintText() {
    if (isset($this->info['type'])) {
      return self::stripHHPrefix($this->info['type']);
    }
    return '';
  }

  public function getTypeText() {
    if (isset($this->info['type_hint'])) {
      return self::stripHHPrefix($this->info['type_hint']);
    }
    return '';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionparameter.isarray.php
   * )
   *
   * Checks if the parameter expects an array.
   *
   * @return     mixed   TRUE if an array is expected, FALSE otherwise.
   */
  public function isArray() {
    return $this->info['type'] == 'array';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.allowsnull.php )
   *
   * Checks whether the parameter allows NULL. Warning: This function is
   * currently not documented; only its argument list is available.
   *
   * @return     mixed   TRUE if NULL is allowed, otherwise FALSE
   */
  public function allowsNull() {
    return isset($this->info['nullable']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.isoptional.php )
   *
   * Checks if the parameter is optional.
   *
   * @return     bool   TRUE if the parameter is optional, otherwise FALSE
   */
  public function isOptional(): bool {
    return !empty($this->info['is_optional']);
  }

  /**
   * Checks if the parameter is variadic.
   *
   * @return     bool   TRUE if the parameter is variadic, otherwise FALSE
   */
  public function isVariadic(): bool {
    return !empty($this->info['is_variadic']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.isdefaultvalueavailable.php
   * )
   *
   * Checks if a default value for the parameter is available.
   *
   * @return     mixed   TRUE if a default value is available, otherwise
   *                     FALSE
   */
  public function isDefaultValueAvailable() {
    if (!array_key_exists('default', $this->info)) {
      return false;
    }
    $defaultValue = $this->info['default'];
    return (!$defaultValue instanceof stdClass);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.getdefaultvalue.php )
   *
   * Gets the default value of the parameter for a user-defined function or
   * method. If the parameter is not optional a ReflectionException will be
   * thrown.
   *
   * @return     mixed   The parameters default value.
   */
  public function getDefaultValue() {
    if (!array_key_exists('default', $this->info)) {
      throw new ReflectionException('Parameter is not optional');
    }
    $defaultValue = $this->info['default'];
    if ($defaultValue instanceof stdclass) {
      throw new ReflectionException($defaultValue->msg);
    }
    return $defaultValue;
  }

  /**
    * @deprecated
    */
  public function getDefaultValueText() {
    return $this->getDefaultValueConstantName();
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.getdefaultvalueconstantname.php
   * )
   *
   * Warning: This function is currently not documented; only its argument
   * list is available.
   *
   * @return     mixed   Returns string on success or NULL on failure.
   */
  public function getDefaultValueConstantName() {
    if (array_key_exists('defaultText', $this->info)) {
      return $this->info['defaultText'];
    }

    return '';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.getposition.php )
   *
   * Gets the position of the parameter.
   *
   * @return     mixed   The position of the parameter, left to right,
   *                     starting at position #0.
   */
  public function getPosition() {
    return $this->info['index'];
  }

  public function getAttribute($name) {
    $attrs = $this->info['attributes'];
    return isset($attrs[$name]) ? $attrs[$name] : null;
  }

  public function getAttributes() {
    return $this->info['attributes'];
  }

  public function getAttributeRecursive($name) {
    $attrs = $this->getAttributesRecursive();
    return isset($attrs[$name]) ? $attrs[$name] : null;
  }

  public function getAttributesRecursive() {
    if (!isset($this->info['class'])) {
      return $this->getAttributes();
    }

    $attrs = array();
    $class = $this->getDeclaringClass();
    $function_name = $this->info['function'];
    $index = $this->info['index'];
    self::collectAttributes(&$attrs, $class, $function_name, $index);
    return $attrs;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionparameter.iscallable.php )
   *
   * Warning: This function is currently not documented; only its argument
   * list is available.
   *
   * @return     mixed   Returns TRUE if the parameter is callable, FALSE if
   *                     it is not or NULL on failure.
   */
  public function isCallable() {
    return $this->getTypeText() === 'callable';
  }

  private static function collectAttributes(&$attrs, $class, $function_name,
                                            $index) {
    if ($class->hasMethod($function_name)) {
      $method = $class->getMethod($function_name);
      $params = $method->getParameters();
      if (count($params) >= $index) {
        $attrs += $params[$index]->getAttributes();
      }
    }

    $parent = $class->getParentClass();
    if ($parent) {
      self::collectAttributes(
        &$attrs,
        $parent,
        $function_name,
        $index);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// property

// This doc comment block generated by idl/sysdoc.php
/**
 * ( excerpt from http://php.net/manual/en/class.reflectionproperty.php )
 *
 * The ReflectionProperty class reports information about classes
 * properties.
 *
 */
class ReflectionProperty implements Reflector {
  const IS_STATIC = 1;
  const IS_PUBLIC = 256;
  const IS_PROTECTED = 512;
  const IS_PRIVATE = 1024;

  public $info;
  public $name;
  public $class;

  private $forceAccessible = false;

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.construct.php
   * )
   *
   * Warning: This function is currently not documented; only its argument
   * list is available.
   *
   * @cls        mixed   The class name, that contains the property.
   * @name       mixed   The name of the property being reflected.
   *
   * @return     mixed   No value is returned.
   */
  public function __construct($cls, $name) {
    if ($cls && $name) {
      $cls = new ReflectionClass($cls);
      $prop = $cls->getProperty($name);
      if ($prop) {
        $this->info  = $prop->info;
        $this->name  = $prop->name;
        $this->class = $prop->class;
      }
    } else {
      throw new ReflectionException("Parameters must not be null");
    }
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.tostring.php
   * )
   *
   * To string. Warning: This function is currently not documented; only its
   * argument list is available.
   *
   */
  public function __toString() {
    return "";
  }

  // Prevent cloning
  public function __clone() {
    throw new BadMethodCallException(
      'Trying to clone an uncloneable object of class ReflectionProperty'
    );
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.export.php )
   *
   * Exports a reflection. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @cls        mixed   The reflection to export.
   * @name       mixed   The property name.
   * @ret        mixed   Setting to TRUE will return the export, as opposed
   *                     to emitting it. Setting to FALSE (the default) will
   *                     do the opposite.
   */
  public static function export($cls, $name, $ret=false) {
    if (!is_object($cls)) {
      $cls = new ReflectionClass($cls);
    } else {
      $cls = new ReflectionClass(get_class($cls));
    }
    $obj = $cls->getProperty($name);
    $str = (string)$obj;
    if ($ret) {
      return $str;
    }
    print $str;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.getname.php )
   *
   * Gets the properties name. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @return     mixed   The name of the reflected property.
   */
  public function getName() {
    return $this->info['name'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.ispublic.php
   * )
   *
   * Checks whether the property is public.
   *
   * @return     mixed   TRUE if the property is public, FALSE otherwise.
   */
  public function isPublic() {
    return $this->info['access'] == 'public';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.isprivate.php
   * )
   *
   * Checks whether the property is private.
   *
   * @return     mixed   TRUE if the property is private, FALSE otherwise.
   */
  public function isPrivate() {
    return $this->info['access'] == 'private';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionproperty.isprotected.php )
   *
   * Checks whether the property is protected.
   *
   * @return     mixed   TRUE if the property is protected, FALSE otherwise.
   */
  public function isProtected() {
    return $this->info['access'] == 'protected';
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.isstatic.php
   * )
   *
   * Checks whether the property is static.
   *
   * @return     mixed   TRUE if the property is static, FALSE otherwise.
   */
  public function isStatic() {
    return isset($this->info['static']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.isdefault.php
   * )
   *
   * Checks whether the property is the default.
   *
   * @return     mixed   TRUE if the property was declared at compile-time,
   *                     or FALSE if it was created at run-time.
   */
  public function isDefault() {
    return $this->info['default'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionproperty.setaccessible.php )
   *
   * Sets a property to be accessible. For example, it may allow protected
   * and private properties to be accessed.
   *
   * @accessible mixed   TRUE to allow accessibility, or FALSE.
   *
   * @return     mixed   No value is returned.
   */
  public function setAccessible($accessible) {
    $this->forceAccessible = $accessible;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionproperty.getmodifiers.php )
   *
   * Gets the modifiers. Warning: This function is currently not documented;
   * only its argument list is available.
   *
   * @return     mixed   A numeric representation of the modifiers.
   */
  public function getModifiers() {
    return $this->info['modifiers'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.getvalue.php
   * )
   *
   * Gets the properties value.
   *
   * @obj        mixed   If the property is non-static an object must be
   *                     provided to fetch the property from. If you want to
   *                     fetch the default property without providing an
   *                     object use ReflectionClass::getDefaultProperties()
   *                     instead.
   *
   * @return     mixed   The current value of the property.
   */
  public function getValue($obj = null) {
    if ($this->isStatic()) {
      return hphp_get_static_property(
        $this->info['class'],
        $this->info['name'],
        $this->forceAccessible
      );
    }
    // Can be removed once we support ParamCoerceMode in PHP
    if (func_num_args() != 1) {
      trigger_error('ReflectionProperty::getValue() expects exactly 1'
        . ' parameter, ' . func_num_args() . ' given', E_USER_WARNING);
      return null;
    }
    // Can be removed once we support ParamCoerceMode in PHP
    if (gettype($obj) != "object") {
      trigger_error('ReflectionProperty::getValue() expects parameter 1'
         . ' to be object, ' . gettype($obj) . ' given', E_USER_WARNING);
      return null;
    }
    return hphp_get_property(
      $obj,
      $this->forceAccessible ? $this->info['class'] : null,
      $this->info['name']
    );
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionproperty.setvalue.php
   * )
   *
   * Sets (changes) the property's value.
   *
   * @obj        mixed   If the property is non-static an object must be
   *                     provided to change the property on. If the property
   *                     is static this parameter is left out and only value
   *                     needs to be provided.
   * @value      mixed   The new value.
   *
   * @return     mixed   No value is returned.
   */
  public function setValue($obj = null, $value = null) {
    if (func_num_args() == 1) {
      $value = $obj;
      $obj = null;
    }
    if ($this->isStatic()) {
      hphp_set_static_property(
        $this->info['class'],
        $this->info['name'],
        $value,
        $this->forceAccessible
      );
    } else {
      // Can be removed once we support ParamCoerceMode in PHP
      if (func_num_args() != 2) {
        trigger_error('ReflectionProperty::setValue() expects exactly 2'
          . ' parameters, ' . func_num_args() . ' given', E_USER_WARNING);
        return null;
      }
      // Can be removed once we support ParamCoerceMode in PHP
      if (gettype($obj) != "object") {
        trigger_error('ReflectionProperty::setValue() expects parameter 1'
          . ' to be object, ' . gettype($obj) . ' given', E_USER_WARNING);
        return null;
      }
      hphp_set_property(
        $obj,
        $this->forceAccessible ? $this->info['class'] : null,
        $this->info['name'],
        $value
      );
    }
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionproperty.getdeclaringclass.php )
   *
   * Gets the declaring class. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @return     mixed   A ReflectionClass object.
   */
  public function getDeclaringClass() {
    if (empty($this->info['class'])) {
      return null;
    }
    return new ReflectionClass($this->info['class']);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionproperty.getdoccomment.php )
   *
   * Gets the doc comment. Warning: This function is currently not
   * documented; only its argument list is available.
   *
   * @return     mixed   The doc comment.
   */
  public function getDocComment() {
    return $this->info['doc'];
  }

  private static function stripHHPrefix($str) {
    if (!is_string($str)) return $str;
    return str_ireplace(
      array('HH\\bool', 'HH\\int', 'HH\\float', 'HH\\string', 'HH\\num',
            'HH\\resource', 'HH\\void', 'HH\\this'),
      array('bool',     'int',     'float',     'string',     'num',
            'resource',     'void', 'this'),
      $str
    );
  }

  public function getTypeText() {
    if (isset($this->info['type'])) {
      return self::stripHHPrefix($this->info['type']);
    }
    return '';
  }
}

///////////////////////////////////////////////////////////////////////////////
// extension

// This doc comment block generated by idl/sysdoc.php
/**
 * ( excerpt from http://php.net/manual/en/class.reflectionextension.php )
 *
 * The ReflectionExtension class reports information about an extension.
 *
 */
class ReflectionExtension implements Reflector {
  private $name;
  private $info;

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.construct.php )
   *
   * Construct a ReflectionExtension object.
   *
   * @name       mixed   Name of the extension.
   *
   * @return     mixed   A ReflectionExtension object.
   */
  public function __construct($name) {
    $this->info = hphp_get_extension_info($name);
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionextension.tostring.php
   * )
   *
   * Exports a reflected extension and returns it as a string. This is the
   * same as the ReflectionExtension::export() with the return set to TRUE.
   *
   * @return     mixed   Returns the exported extension as a string, in the
   *                     same way as the ReflectionExtension::export().
   */
  public function __toString() {
    return "";
  }

  // Prevent cloning
  public function __clone() {
    throw new BadMethodCallException(
      'Trying to clone an uncloneable object of class ReflectionExtension'
    );
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionextension.export.php )
   *
   * Exports a reflected extension. The output format of this function is
   * the same as the CLI argument --re [extension].
   *
   * @name       mixed   The reflection to export.
   * @ret        mixed   Setting to TRUE will return the export, as opposed
   *                     to emitting it. Setting to FALSE (the default) will
   *                     do the opposite.
   *
   * @return     mixed   If the return parameter is set to TRUE, then the
   *                     export is returned as a string, otherwise NULL is
   *                     returned.
   */
  public static function export($name, $ret=false) {
    $obj = new ReflectionExtension($name);
    $str = (string)$obj;
    if ($ret) {
      return $str;
    }
    print $str;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionextension.getname.php
   * )
   *
   * Gets the extensions name.
   *
   * @return     mixed   The extensions name.
   */
  public function getName() {
    return $this->info['name'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getversion.php )
   *
   * Gets the version of the extension.
   *
   * @return     mixed   The version of the extension.
   */
  public function getVersion() {
    return $this->info['version'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getfunctions.php )
   *
   * Get defined functions from an extension.
   *
   * @return     mixed   An associative array of ReflectionFunction objects,
   *                     for each function defined in the extension with the
   *                     keys being the function names. If no function are
   *                     defined, an empty array is returned.
   */
  public function getFunctions() {
    return $this->info['functions'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getconstants.php )
   *
   * Get defined constants from an extension.
   *
   * @return     mixed   An associative array with constant names as keys.
   */
  public function getConstants() {
    return $this->info['constants'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getinientries.php )
   *
   * Get the ini entries for an extension.
   *
   * @return     mixed   An associative array with the ini entries as keys,
   *                     with their defined values as values.
   */
  public function getINIEntries() {
    return $this->info['ini'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getclasses.php )
   *
   * Gets a list of classes from an extension.
   *
   * @return     mixed   An array of ReflectionClass objects, one for each
   *                     class within the extension. If no classes are
   *                     defined, an empty array is returned.
   */
  public function getClasses() {
    return $this->info['classes'];
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from
   * http://php.net/manual/en/reflectionextension.getclassnames.php )
   *
   * Gets a listing of class names as defined in the extension.
   *
   * @return     mixed   An array of class names, as defined in the
   *                     extension. If no classes are defined, an empty array
   *                     is returned.
   */
  public function getClassNames() {
    $ret = array();
    foreach ($this->info['classes'] as $cls) {
      $ret[] = $cls->getName();
    }
    return $ret;
  }

  // This doc comment block generated by idl/sysdoc.php
  /**
   * ( excerpt from http://php.net/manual/en/reflectionextension.info.php )
   *
   * Prints out the " phpinfo()" snippet for the given extension.
   *
   * @return     mixed   Information about the extension.
   */
  public function info() {
    return $this->info['info'];
  }
}
