<?php

/**
 * @file
 * Administrative page callbacks for menu_import module.
 */

/**
 * Main form for menu file upload and menu preview.
 */
function menu_import_form($form, &$form_state) {
  if (isset($form_state['storage']['step'])) {
    $step = $form_state['storage']['step'];
  }
  else {
    $step = 1;
  }

  $form = array();
  // Call the step-specific form alters.
  $function = "menu_import_form_step$step";
  if (function_exists($function)) {
    $function($form, $form_state);
  }

  return $form;
}

/**
 * Step 1 of menu import process.
 */
function menu_import_form_step1(&$form, &$form_state) {
  $form['main'] = array(
    '#type' => 'fieldset',
    '#title' => t('General settings'),
  );
  $form['main']['menu_name'] = array(
    '#type' => 'select',
    '#title' => t('Menu to import in'),
    '#options' => menu_get_menus(),
    '#description' => t('This menu will contain the imported items.'),
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['menu_name']) ? $form_state['values']['menu_name'] : NULL,
  );
  $form['main']['upload'] = array(
    '#type' => 'file',
    '#title' => t('Menu file'),
    '#description' => t('File containing specially indented text.'),
  );

  $form['node'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content settings'),
  );

  $node_types = node_type_get_names();
  if (empty($node_types)) {
    drupal_set_message(t('No content types were found, import is not possible.'), 'error', FALSE);
  }

  $form['node']['node_type'] = array(
    '#type' => 'select',
    '#title' => t('Content type'),
    '#options' => $node_types,
    '#description' => t('Type of the newly created content.'),
    '#required' => TRUE,
    '#default_value' => !empty($form_state['values']['node_type']) ? $form_state['values']['node_type'] : NULL,
  );

  if (module_exists('locale')) {
    $language_list = locale_language_list();
    $form['node']['node_language'] = array(
      '#type' => 'select',
      '#title' => t('Content language'),
      '#options' => $language_list,
      '#description' => t('Language of the newly created content.'),
      '#required' => TRUE,
      '#default_value' => !empty($form_state['values']['node_language']) ? $form_state['values']['node_language'] : NULL,
    );
  }
  else {
    $form['node']['node_language'] = array(
      '#type' => 'hidden',
      '#value' => LANGUAGE_NONE,
    );
  }

  $form['node']['node_status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Published'),
    '#description' => t('Check to automatically publish the created content.'),
    '#default_value' => !empty($form_state['values']['node_status']) ? $form_state['values']['node_status'] : 1,
  );

  // Style this element differently.
  drupal_add_css(
    '.hint {border: 1px dashed #CCCCCC; padding: 5px; display: inline-block;}',
    'inline');
  drupal_add_js(
    '(function($){
      $(function(){
        $("input[name=node_attach]").change(function(){
          $("div[id^=\'mi-help\']").hide();
          $("#mi-help-" + $(this).val()).show();
        });
        $("input[id=\'edit-node-attach-0\']").trigger("change");
      });
    })(jQuery)',
    'inline');

  $form['node']['node_attach'] = array(
    '#type' => 'radios',
    '#title' => t('Content relation'),
    '#options' => array(
      MI_NODE_LINK => t('Link to existing content'),
      MI_NODE_LINK_CREATE => t('Link to existing and create empty content'),
      MI_NODE_RECREATE => t('Remove existing and create empty content'),
    ),
    '#description' =>
      '<div class="hint" id="mi-help-' . MI_NODE_LINK . '">' . t('Look for existing content by either path or title.<br/>If content was not found create menu links pointing to &lt;front&gt.') . '</div>'
      . '<div class="hint" id="mi-help-' . MI_NODE_LINK_CREATE . '">' . t('Look for existing content by either path or title.<br/>If content was not found it will be created as stub.') . '</div>'
      . '<div class="hint" id="mi-help-' . MI_NODE_RECREATE . '">' . t('Remove existing menu items and content referenced by them,<br/>and then apply the previous option. Use carefully to evade data loss!') . '</div>',
    '#default_value' => !empty($form_state['values']['node_attach']) ? $form_state['values']['node_attach'] : MI_NODE_LINK,
  );

  $form['node']['search_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('Search content by title'),
    '#description' => t('Content will be searched by title. The first content found with given title will be used.'),
    '#default_value' => !empty($form_state['values']['search_title']) ? $form_state['values']['search_title'] : 0,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['sumbit'] = array(
    '#type' => 'submit',
    '#value' => t('Upload & preview'),
    '#submit' => array('menu_import_form_step1_submit'),
    '#validate' => array('menu_import_form_step1_validate'),
  );
}

/**
 * Step 1 form validate handler.
 */
function menu_import_form_step1_validate($form, &$form_state) {
  $validators = array(
    'file_validate_extensions' => array('txt'),
  );
  $file = file_save_upload('upload', $validators);
  if (!$file) {
    form_set_error('upload', t('You must select a valid text file to upload.'));
  }
  else {
    $form_state['values']['file'] = $file;
  }
}

/**
 * Step 1 form submit handler.
 */
function menu_import_form_step1_submit($form, &$form_state) {
  $file = $form_state['values']['file'];

  $form_state['storage']['options'] = array(
//    'search_alias' => $form_state['values']['search_alias'],
    'search_title' => $form_state['values']['search_title'],
    'node_language' => $form_state['values']['node_language'],
    'node_type' => $form_state['values']['node_type'],
    'node_attach' => $form_state['values']['node_attach'],
    'node_status' => $form_state['values']['node_status'],
  );

  module_load_include('inc', 'menu_import', 'includes/import');
  $menu = menu_import_parse_menu_from_file($file->uri, $form_state['values']['menu_name'], $form_state['storage']['options']);
  file_delete($file);

  if ($menu['errors']) {
    foreach ($menu['errors'] as $error) {
      drupal_set_message(check_plain($error), 'warning');
    }
  }
  $form_state['storage']['menu'] = $menu;
  $form_state['storage']['step'] = 2;
  $form_state['rebuild'] = TRUE;
  $form_state['page_values'][1] = $form_state['values'];
}

/**
 * Step 2.
 */
function menu_import_form_step2(&$form, &$form_state) {
  $menu = $form_state['storage']['menu'];
  $options = $form_state['storage']['options'];

  $form['menu'] = menu_import_preview($menu, $options);

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['back'] = array(
    '#type' => 'submit',
    '#value' => t('Back'),
    '#submit' => array('menu_import_form_back_submit'),
    '#weight' => -1,
    '#limit_validation_errors' => array(),
  );
  $form['actions']['sumbit'] = array(
    '#type' => 'submit',
    '#value' => t('Import'),
    '#submit' => array('menu_import_form_step2_submit'),
  );
}

/**
 * Step 2 form submit handler.
 */
function menu_import_form_step2_submit($form, &$form_state) {
  $menu = $form_state['storage']['menu'];
  $options = $form_state['storage']['options'];
  $menu_name = $menu[0]['menu_name'];

  module_load_include('inc', 'menu_import', 'includes/import');
  $result = menu_import_save_menu($menu, $options);
  menu_import_display_result($result);

  $form_state['redirect'] = 'admin/structure/menu/manage/' . $menu_name;
}

/**
 * Helper function to display result.
 */
function menu_import_display_result(array $result) {
  $total_items = $result['new_nodes'] + $result['matched_nodes'] + $result['external_links'] + $result['unknown_links'];
  drupal_set_message(t('Items imported: @items.', array('@items' => $total_items)));

  // Show error messages.
  if ($result['failed']) {
    drupal_set_message(t('Menu item creation failed: @count items.', array('@count' => $result['failed'])), 'error');
  }
  
  // Show OK messages.
  if ($result['new_nodes']) {
    drupal_set_message(t('New content created: @count items.', array('@count' => $result['new_nodes'])));
  }
  if ($result['matched_nodes']) {
    drupal_set_message(t('Existing content matched: @count items.', array('@count' => $result['matched_nodes'])));
  }
  if ($result['deleted_nodes']) {
    drupal_set_message(t('Content deleted: @count items.', array('@count' => $result['deleted_nodes'])));
  }

  // Show additional statistic messages.
  if ($result['external_links']) {
    drupal_set_message(t('External URLs: @count items.', array('@count' => $result['external_links'])));
  }
  if ($result['unknown_links']) {
    drupal_set_message(t('Content not found: @count items.', array('@count' => $result['unknown_links'])));
  }
}

/**
 * Back button submit handler.
 */
function menu_import_form_back_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['values'] = $form_state['page_values'][1];
  $form_state['storage']['step'] = 1;
}

/**
 * Renders the menu for preview.
 */
function menu_import_preview($menu, array $options) {
  $menu_name = $menu[0]['menu_name'];
  $menus = menu_get_menus();

  $descriptions = array();
  $descriptions[] = t('Import @count items into "@menu_title" menu (@menu_name)',
    array(
      '@count' => count($menu) - 2, // Minus errors and root item.
      '@menu_title' => $menus[$menu_name],
      '@menu_name' => $menu_name
  ));

  switch ($options['node_attach']) {
    case MI_NODE_LINK:
      $descriptions[] = t('Link to existing content');
      break;
    case MI_NODE_LINK_CREATE:
      $descriptions[] = t('Link to existing and create empty content');
      break;
    case MI_NODE_RECREATE:
      $descriptions[] = t('Remove existing and create empty content');
      break;
  }

  if ($options['node_language'] != LANGUAGE_NONE) {
    $language = module_exists('locale') ? locale_language_name($options['node_language']) : $options['node_language'];
    $descriptions[] = t('Content language: @language', array('@language' => $language));
  }

  $status = $options['node_status'] ? t('published') : t('not published');
  $descriptions[] = t('Content status: @status', array('@status' => $status));

  $rows = array();
  $depth = 0;
  foreach ($menu[0]['children'] as $index) {
    $new_rows = _menu_import_preview_row_recurse($menu, $index, $depth);
    foreach ($new_rows as $row) {
      $rows[] = $row;
    }
  }

  $build['options'] = array(
    '#theme' => 'item_list',
    '#items' => $descriptions,
    '#title' => t('Import options'),
    '#type' => 'ul',
  );
  $build['menu'] = array(
    '#theme' => 'table',
    '#header' => array(t('Menu Item'), t('Content exists'), t('Path'), t('System path'), t('Weight')),
    '#rows' => $rows,
    '#caption' => t('Menu preview'),
  );
  return $build;
}

/**
 * Recursive function for theming a menu item and its children.
 */
function _menu_import_preview_row_recurse($menu, $index, $level) {
  $rows = array();

  $title = theme('indentation', array('size' => $level));
  $title .= check_plain($menu[$index]['link_title']);
  $exists = $menu[$index]['nid'] ? t('Yes') : t('No');
  $path = isset($menu[$index]['path']) && !empty($menu[$index]['path'])
            ? $menu[$index]['path'] : ' - ';
  $link_path = isset($menu[$index]['link_path']) && !empty($menu[$index]['link_path'])
                ? $menu[$index]['link_path'] : ' - ';
  $weight = $menu[$index]['weight'];

  $rows[] = array(
    'data' => array(
      array('data' => $title),
      array('data' => $exists),
      array('data' => check_url($path)),
      array('data' => check_url($link_path)),
      array('data' => $weight),
    )
  );

  foreach ($menu[$index]['children'] as $child) {
    $new_rows = _menu_import_preview_row_recurse($menu, $child, ($level +1));
    foreach ($new_rows as $row) {
      $rows[] = $row;
    }
  }
  return $rows;
}
