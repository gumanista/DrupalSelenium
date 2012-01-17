<?php

/**
 * Test case for Selenium test.
 */
class DrupalSeleniumWebTestCase extends DrupalWebTestCase {

  /**
   * Selenium Firefox Driver instance.
   *
   * @var SeleniumWebdriver
   */
  protected $driver;

  /**
   * Selenium Firefox Driver instance.
   *
   * @var string
   */
  public $browser = 'firefox';

  /**
   * Allowed driver types.
   */
  protected $allowed_browsers = array('firefox', 'chrome');

  protected function setUp() {
    $modules = func_get_args();
    parent::setUp($modules);

    $this->driver = $this->seleniumDriver($this->browser);
  }

  /**
   * Init driver of specified type.
   *
   * @param string $browser
   *   Type of the driver.
   * @return object
   */
  protected function seleniumDriver($browser) {
    switch ($browser) {
      case 'firefox':
        return new SeleniumFirefoxDriver();
      case 'chrome':
        return new SeleniumChromeDriver();
    }
  }

  /**
   * Open specific url.
   */
  protected function drupalGet($url) {
    $this->driver->openUrl($url);
  }

  /**
   * Login with current user.
   */
  protected function drupalLogin($user) {
    if ($this->loggedInUser) {
      $this->drupalLogout();
    }

    $this->drupalGet('user');

    $this->driver->getElement('css=#edit-name')->sendKeys($user->name);
    $this->driver->getElement('css=#edit-pass')->sendKeys($user->pass_raw);
    $this->driver->getElement('css=#edit-submit')->submit();

    // If a "log out" link appears on the page, it is almost certainly because
    // the login was successful.
    $pass = $this->assertLink(t('Log out'), 0, t('User %name successfully logged in.', array('%name' => $user->name)), t('User login'));

    if ($pass) {
      $this->loggedInUser = $user;
    }
  }

  /**
   * Logs a user out.
   */
  protected function drupalLogout() {
    // Make a request to the logout page, and redirect to the user page, the
    // idea being if you were properly logged out you should be seeing a login
    // screen.

    $this->drupalGet('user/logout');
    $this->drupalGet('user');

    $pass = $this->assertField('name', t('Username field found.'), t('Logout'));

    $pass = $pass && $this->assertField('pass', t('Password field found.'), t('Logout'));

    if ($pass) {
      $this->loggedInUser = FALSE;
    }
  }

  /**
   * Take a screenshot from current page.
   * Save it to verbose directory and add verbose message.
   */
  protected function verboseScreenshot() {
    // Take screenshot of current page.
    $screenshot = FALSE;
    try {
      $screenshot = $this->driver->getScreenshot();
    }
    catch (Exception $e) {
      $this->verbose(t('No support for screenshots in %driver', array('%driver' => get_class($this->driver))));
    }
    if ($screenshot) {
      // Prepare directory.
      $directory = $this->originalFileDirectory . '/simpletest/verbose/screenshots';
      $writable = file_prepare_directory($directory, FILE_CREATE_DIRECTORY);
      if ($writable) {
        $testname = $this->getTestName();
        // Trying to save screenshot to verbose directory.
        $file = file_unmanaged_save_data($screenshot, $this->originalFileDirectory . '/simpletest/verbose/screenshots/' . $testname . '.png', FILE_EXISTS_RENAME);

        // Adding verbose message with link to screenshot.
        $this->error(l(t('Screenshot created.'), $GLOBALS['base_url'] . '/' . $file, array('attributes' => array('target' => '_blank'))), 'User notice');
      }
    }
  }

  /**
   * Implements assertTextHelper.
   */
  protected function assertTextHelper($text, $message = '', $group, $not_exists) {
    $this->plainTextContent = filter_xss($this->driver->getBodyText(), array());

    // Remove all symbols of new line as we need raw text here.
    $this->plainTextContent = str_replace("\n", '', $this->plainTextContent);

    if (!$message) {
      $message = !$not_exists ? t('"@text" found', array('@text' => $text)) : t('"@text" not found', array('@text' => $text));
    }
    return $this->assert($not_exists == (strpos($this->plainTextContent, $text) === FALSE), $message, $group);
  }

  /**
   * Implements assertTitle.
   */
  protected function assertTitle($title, $message = '', $group = 'Other') {
    $actual = $this->driver->getPageTitle();
    if (!$message) {
      $message = t('Page title @actual is equal to @expected.', array(
        '@actual' => var_export($actual, TRUE),
        '@expected' => var_export($title, TRUE),
      ));
    }
    return $this->assertEqual($actual, $title, $message, $group);
  }

  /**
   * Asserts that a field exists with the given name or id.
   *
   * @param type $field
   *   Name or id of field to assert.
   * @param type $message
   *   Message to display.
   * @param type $group
   *   The group this message belongs to.
   * @return type
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertField($field, $message = '', $group = 'Other') {
    try {
      $element = $this->driver->getElement("name=$field");
    } catch (Exception $e) {
      try {
        $element = $this->driver->getElement("id=$field");
      } catch (Exception $e) {
        $element = FALSE;
      }
    }
    return $this->assertTrue(!empty($element), $message ? $message : t('Field %locator found', array('%locator' => $field)), $group);
  }

  /**
    * Implements assertNoField.
    *
    * @param $field
    *   Name or id of field to assert.
    * @param $message
    *   Message to display.
    * @param $group
    *   The group this message belongs to.
    * @return
    *   TRUE on pass, FALSE on fail.
    */
   protected function assertNoField($field, $message = '', $group = 'Other') {
     try {
       $element = $this->driver->getElement("name=$field");
     }
     catch (Exception $e) {
       try {
         $element = $this->driver->getElement("id=$field");
       }
       catch (Exception $e) {
         $element = FALSE;
       }
    }
    return $this->assertTrue(empty($element), $message ? $message : t('Field %locator not found', array('%locator' => $field)), $group);
  }

  /**
   * Implements assertLink.
   */
  protected function assertLink($label, $index = 0, $message = '', $group = 'Other') {
    $links = $this->driver->waitForElements('link=' . $label);
    $message = ($message ?  $message : t('Link with label %label found.', array('%label' => $label)));
    return $this->assert(isset($links[$index]), $message, $group);
  }

  /**
   * Follows a link by name.
   *
   * Will click the first link found with this link text by default, or a
   * later one if an index is given. Match is case insensitive with
   * normalized space. The label is translated label. There is an assert
   * for successful click.
   *
   * @param $label
   *   Text between the anchor tags.
   * @param $index
   *   Link position counting from zero.
   * @return
   *   Page on success, or FALSE on failure.
   */
  protected function clickLink($label, $index = 0) {
    // Assert that link exists.
    if (!$this->assertLink($label, $index)) {
      return;
    }

    // Get link elements.
    $links = $this->driver->waitForElements('link=' . $label);

    /**
     * @param $link_element SeleniumWebElement
     */
    $link_element = $links[$index];

    // Get current and target urls.
    $url_before = $this->getUrl();
    $url_target = $link_element->getAttributeValue('href');

    $this->assertTrue(isset($links[$index]), t('Clicked link %label (@url_target) from @url_before', array('%label' => $label, '@url_target' => $url_target, '@url_before' => $url_before)), t('Browser'));

    // Click on element;
    $link_element->click();
  }

  /**
   * Pass if a link with the specified label is found, and optional with the
   * specified index.
   *
   * @param $label
   *   Text between the anchor tags.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNoLink($label, $index = 0, $message = '', $group = 'Other') {
    $links = $this->driver->waitForElements('link=' . $label);
    $message = ($message ?  $message : t('Link with label %label not found.', array('%label' => $label)));
    return $this->assert(!isset($links[$index]), $message, $group);
  }

  /**
   * Pass if a link containing a given href (part) is found.
   *
   * @param $href
   *   The full or partial value of the 'href' attribute of the anchor tag.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertLinkByHref($href, $index = 0, $message = '', $group = 'Other') {
    $links = $this->driver->getAllElements("//a[contains(@href, '$href')]");
    $message = ($message ?  $message : t('Link containing href %href found.', array('%href' => $href)));
    return $this->assert(isset($links[$index]), $message, $group);
  }

  /**
   * Pass if a link containing a given href (part) is not found.
   *
   * @param $href
   *   The full or partial value of the 'href' attribute of the anchor tag.
   * @param $index
   *   Link position counting from zero.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE if the assertion succeeded, FALSE otherwise.
   */
  protected function assertNoLinkByHref($href, $index = 0, $message = '', $group = 'Other') {
    $links = $this->driver->getAllElements("//a[contains(@href, '$href')]");
    $message = ($message ?  $message : t('Link containing href %href not found.', array('%href' => $href)));
    return $this->assert(!isset($links[$index]), $message, $group);
  }

  /**
   * Implements assertOptionSelected.
   * Asserts that a select option in the current page is checked.
   *
   * @param type $locator
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   *
   * @todo $id is unusable. Replace with $name.
   */
  protected function assertOptionSelected($locator, $option, $message = '') {
    $selected = FALSE;
    $element = $this->driver->getElement($locator);
    $is_select = $element && $element->getTagName() == 'select';
    if ($is_select) {
      $id = $element->getAttributeValue('id');
      $message = $message ? $message : t('Option @option for field @id is selected.', array('@option' => $option, '@id' => $id));
      $selected_options = $this->getSelectedItem($element);
      foreach ($selected_options as $selected_option) {
        if ($selected_option->getValue() == $option) {
          $selected = TRUE;
          break;
        }
      }
    }
    else {
      $message = t('There is no element with locator @locator or element is not select list.', array('@locator' => $locator));
    }

    return $this->assertTrue($is_select && $selected, $message, t('Browser'));
  }

  /**
   * Implements assertNoOptionSelected.
   * Asserts that a select option in the current page is not checked.
   *
   * @param type $locator
   * @param $option
   *   Option to assert.
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoOptionSelected($locator, $option, $message = '') {
    $selected = FALSE;
    $element = $this->driver->getElement($locator);
    $is_select = $element && $element->getTagName() == 'select';
    if ($is_select) {
      $id = $element->getAttributeValue('id');
      $message = $message ? $message : t('Option @option for field @id is not selected.', array('@option' => $option, '@id' => $id));
      $selected_options = $this->getSelectedItem($element);
      foreach ($selected_options as $selected_option) {
        if ($selected_option->getValue() == $option) {
          $selected = TRUE;
          break;
        }
      }
    }
    else {
      $message = t('There is no element with locator @locator or element is not select list.', array('@locator' => $locator));
    }

    return $this->assertTrue($is_select && !$selected, $message, t('Browser'));
  }

  /**
   * Implements assertFieldChecked.
   * Asserts that a checkbox field in the current page is checked.
   *
   * @param type $locator
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldChecked($locator, $message = '') {
    $element = $this->driver->getElement($locator);
    $is_checkbox = $element && ($element->getTagName() == 'checkbox' || $element->getAttributeValue('type') == 'checkbox');
    if ($is_checkbox) {
      $id = $element->getAttributeValue('id');
      $message = $message ? $message : t('Checkbox field @id is checked.', array('@id' => $id));
    }
    else {
      $message = t('There is no element with locator @locator or element is not checkbox.', array('@locator' => $locator));
    }

    return $this->assertTrue($is_checkbox && $element->isSelected() , $message, t('Browser'));
  }

  /**
   * Implements assertNoFieldChecked.
   * Asserts that a checkbox field in the current page is not checked.
   *
   * @param type $locator
   * @param $message
   *   Message to display.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldChecked($locator, $message = '') {
    $element = $this->driver->getElement($locator);
    $is_checkbox = $element && ($element->getTagName() == 'checkbox' || $element->getAttributeValue('type') == 'checkbox');
    if ($is_checkbox) {
      $id = $element->getAttributeValue('id');
      $message = $message ? $message : t('Checkbox field @id is not checked.', array('@id' => $id));
    }
    else {
      $message = t('There is no element with locator @locator or element is not checkbox.', array('@locator' => $locator));
    }

    return $this->assertTrue($is_checkbox && !$element->isSelected() , $message, t('Browser'));
  }

  /**
   * Asserts that each HTML ID is used for just a single element.
   *
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @param $ids_to_skip
   *   An optional array of ids to skip when checking for duplicates. It is
   *   always a bug to have duplicate HTML IDs, so this parameter is to enable
   *   incremental fixing of core code. Whenever a test passes this parameter,
   *   it should add a "todo" comment above the call to this function explaining
   *   the legacy bug that the test wishes to ignore and including a link to an
   *   issue that is working to fix that legacy bug.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoDuplicateIds($message = '', $group = 'Other', $ids_to_skip = array()) {
    try {
      $elements = $this->driver->getAllElements("//*[@id]");
      $status = TRUE;
      foreach ($elements as $element) {
        $id = (string) $element->getAttributeValue("id");
        if (isset($seen_ids[$id]) && !in_array($id, $ids_to_skip)) {
          $this->fail(t('The HTML ID %id is unique.', array('%id' => $id)), $group);
          $status = FALSE;
        }
        $seen_ids[$id] = TRUE;
      }
    }
    catch (Exception $e) {
      $status = FALSE;
    }
    return $this->assertTrue($status, $message ? $message : t('No Duplicate Ids'), $group);
  }

  /**
   * Asserts that a field exists in the current page with the given name and value.
   *
   * @param $name
   *   Name of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldByName($name, $value = '', $message = '', $group = 'Other') {
    try {
      $element = $this->driver->getElement("name=$name");
      if ($value) {
        $element = $this->elementValue($element, $value);
      }
    }
    catch (Exception $e) {
      $element = FALSE;
    }
    return $this->assertTrue(!empty($element), $message ? $message : t('Field found by name'), $group);
  }

  /**
   * Asserts that a field not exists in the current page with the given name and value.
   *
   * @param $name
   *   Name of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldByName($name, $value = '', $message = '', $group = 'Other') {
    try {
      $element = $this->driver->getElement("name=$name");
      if ($value) {
        $element = $this->elementValue($element, $value);
      }
    }
    catch (Exception $e) {
      $element = FALSE;
    }
    return $this->assertTrue(empty($element), $message ? $message : t('Field found by name'), $group);
  }

  /**
   * Asserts that a field exists in the current page with the given id and value.
   *
   * @param $id
   *   Id of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldById($id, $value = '', $message = '', $group = 'Other') {
    try {
      $element = $this->driver->getElement("id=$id");
      if ($value) {
        $element = $this->elementValue($element, $value);
      }
    }
    catch (Exception $e) {
      $element = FALSE;
    }
    return $this->assertTrue(!empty($element), $message ? $message : t('Field found by id'), $group);
  }

  /**
   * Asserts that a field not exists in the current page with the given id and value.
   *
   * @param $id
   *   Id of field to assert.
   * @param $value
   *   Value of the field to assert.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoFieldById($id, $value = '', $message = '', $group = 'Other') {
    try {
      $element = $this->driver->getElement("id=$id");
      if ($value) {
        $element = $this->elementValue($element, $value);
      }
    }
    catch (Exception $e) {
      $element = FALSE;
    }
    return $this->assertTrue(empty($element), $message ? $message : t('Field found by id'), $group);
  }

  /**
   * Check the value of the form element.
   *
   * @param type $element
   * @param type $value
   *
   *
   */
  protected function elementValue(SeleniumWebElement $element, $value) {
    $result = TRUE;
    switch ($element->getTagName()) {
      case 'input':
        $element_value = $element->getValue();
        break;
      case 'textarea':
        $element_value = $element->getText();
        break;
      case 'select':
        $element_value = $element->getSelected()->getValue();
        $element_text = $element->getSelected()->getText();
        break;
      default : {
        $result = FALSE;
      }
    }
    return (($result) && ($value == $element_value || $value == $element_text));
  }

  /**
   * Asserts that a field exists in the current page by the given XPath.
   *
   * @param $xpath
   *   XPath used to find the field.
   * @param $value
   *   (optional) Value of the field to assert.
   * @param $message
   *   (optional) Message to display.
   * @param $group
   *   (optional) The group this message belongs to.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertFieldByXPath($xpath, $value = NULL, $message = '', $group = 'Other') {
      try {
      $element = $this->driver->getElement($xpath);
      if ($value) {
        $element = $this->elementValue($element, $value);
      }
    }
    catch (Exception $e) {
      $element = FALSE;
    }
    return $this->assertTrue(!empty($element), $message ? $message : t('Field found by Xpath'), $group);
  }

  /**
   * Execute a POST request on a Drupal page.
   * It will be done as usual POST request with SimpleBrowser.
   *
   * @param $path
   *   Location of the post form. Either a Drupal path or an absolute path or
   *   NULL to post to the current page. For multi-stage forms you can set the
   *   path to NULL and have it post to the last received page. Example:
   *
   *   @code
   *   // First step in form.
   *   $edit = array(...);
   *   $this->formSubmit('some_url', $edit, t('Save'));
   *
   *   // Second step in form.
   *   $edit = array(...);
   *   $this->formSubmit(NULL, $edit, t('Save'));
   *   @endcode
   * @param  $edit
   *   Field data in an associative array. Changes the current input fields
   *   (where possible) to the values indicated. A checkbox can be set to
   *   TRUE to be checked and FALSE to be unchecked. Note that when a form
   *   contains file upload fields, other fields cannot start with the '@'
   *   character.
   *
   *   Multiple select fields can be set using name[] and setting each of the
   *   possible values. Example:
   *   @code
   *   $edit = array();
   *   $edit['name[]'] = array('value1', 'value2');
   *   @endcode
   * @param $submit
   *   Value of the submit button whose click is to be emulated. For example,
   *   t('Save'). The processing of the request depends on this value. For
   *   example, a form may have one button with the value t('Save') and another
   *   button with the value t('Delete'), and execute different code depending.
   */
  protected function drupalPost($path, $edit, $submit, $disable_js = array()) {
    if ($this->getUrl() != $path && !is_null($path)) {
      $this->drupalGet($path);
    }

    // Disable javascripts that hide elements.
    $this->disableJs($disable_js);

    // Find form elements and set the values.
    foreach ($edit as $selector => $value){
      $element = $this->driver->getElement("name=$selector");
      // Type of input element. Can be textarea, select or input. If input,
      // we need to check 'type' property.
      $type = $element->getTagName();
      if ($type == 'input') {
        $type = $element->getAttributeValue('type');
      }
      switch ($type) {
        case 'text':
        case 'textarea':
          // Clear element first then send text data.
          $element->clear();
          $element->sendKeys($value);
          break;
        case 'select':
          $element->selectValue($value);
          break;
        case 'radio':
          $elements = $this->driver->getAllElements("name=$selector");
          foreach ($elements as $element) {
            if ($element->getValue() == $value) {
              $element->click();
            }
          }
          break;
        case 'checkbox':
          $elements = $this->driver->getAllElements("name=$selector");
          if (!is_array($value)) {
            $value = array($value);
          }
          foreach ($elements as $element) {
            $element_value = $element->getValue();
            $element_selected = $element->isSelected();
            // Click on element if it should be selected but isn't or if element
            // shouldn't be selected but it is.
            if ((in_array($element_value, $value) && !$element_selected) ||
                (!in_array($element_value, $value) && $element_selected)) {
              $element->click();
            }
          }
          break;
      }
    }

    // Find button and submit the form.
    $elements = $this->driver->getAllElements("name=op");
    foreach ($elements as $element) {
      $val = $element->getValue();
      if ($val == $submit){
        $element->submit();
        break;
      }
    }

    // Wait for the page to load.
    $this->driver->waitForElements('css=body');
  }

  /**
   * Injects javascript code to disable work of some of the drupal javascripts.
   *
   * For example vertical tabs hides some of the elements on the node form.
   * This leads to situation when Selenium can't access to hidden fields. So if
   * we use drupalPost method that should behave similar to native simpletest
   * method we are not able to submit the form properly.
   *
   * @param array $scripts
   */
  function disableJs($scripts) {
    $scripts += array(
      'vertical tabs' => TRUE,
    );

    foreach ($scripts as $type => $execute) {
      if (!$execute) {
        continue;
      }
      $javascript = '';
      switch ($type) {
        case 'vertical tabs':
          $javascript = 'jQuery(".vertical-tabs-pane").show();';
          break;
      }
      // Inject javascript.
      if (!empty($javascript)) {
        $this->driver->executeJsSync($javascript);
      }
    }
  }

  /**
   * Get name of current test running.
   *
   * @return string
   */
  protected function getTestName() {
    $backtrace = debug_backtrace();
    foreach ($backtrace as $bt_item) {
      if (strtolower(substr($bt_item['function'], 0, 4)) == 'test') {
        return $bt_item['function'];
      }
    }
  }

  /**
   * Implements getSelectedItem.
   * Get the selected value from a select field.
   *
   * @param $element
   *   SimpleXMLElement select element.
   * @return
   *   The selected options array.
   */
  protected function getSelectedItem(SeleniumWebElement $element) {
    $result = array();
    foreach ($element->getOptions() as $option) {
      if ($option->isSelected()) {
        $result[] = $option;
      }
    }
    return $result;
  }

  /**
   * Pass if the browser's URL matches the given path.
   *
   * @param $path
   *   The expected system path.
   * @param $options
   *   (optional) Any additional options to pass for $path to url().
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to, defaults to 'Other'.
   *
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertUrl($path, array $options = array(), $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Current URL is @url.', array(
        '@url' => var_export(url($path, $options), TRUE),
      ));
    }
    $options['absolute'] = TRUE;
    return $this->assertEqual($this->getUrl(), url($path, $options), $message, $group);
  }

  /**
   * Pass if the page title is not the given string.
   *
   * @param $title
   *   The string the title should not be.
   * @param $message
   *   Message to display.
   * @param $group
   *   The group this message belongs to.
   * @return
   *   TRUE on pass, FALSE on fail.
   */
  protected function assertNoTitle($title, $message = '', $group = 'Other') {
    if (!$message) {
      $message = t('Page title @actual is not equal to @unexpected.', array(
        '@actual' => var_export($this->driver->getPageTitle(), TRUE),
        '@unexpected' => var_export($title, TRUE),
      ));
    }
    return $this->assertNotEqual($this->driver->getPageTitle(), $title, $message, $group);
  }

  /**
   * Gets the current raw HTML of requested page.
   */
  protected function drupalGetContent() {
    return $this->driver->getBodyText();
  }

  /**
   * Get the current url of the browser.
   *
   * @return
   *   The current url.
   */
  protected function getUrl() {
    return $this->driver->getUrl();
  }
}
