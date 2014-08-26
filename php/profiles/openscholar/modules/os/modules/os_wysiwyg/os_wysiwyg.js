/**
 * Hides the text format help when the wysiwyg is disabled
 */
Drupal.behaviors.osWysiwygHideTips = {
  attach: function (ctx) {
    // don't do this for every thing
    if (ctx != document) return;
    var $ = jQuery;
    function toggle (e) {
      $(e.currentTarget).parents('.text-format-wrapper').find('.filter-wrapper').toggle();
    }
    $('.filter-wrapper', ctx).hide();
    $('.wysiwyg-toggle-wrapper a', ctx).die('click', toggle).live('click', toggle);
  }
};