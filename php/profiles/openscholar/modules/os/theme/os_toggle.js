/**
 * Pairs a link and a block of content. The link will toggle the appearance of
 * that block content
 *
 */
Drupal.behaviors.os_toggle = {
  attach: function (ctx) {
    var $ = jQuery;
    $('.toggle', ctx).click(function(){
      $(this).toggleClass("expanded")

      if (!$.browser.msie) {
        $(this).nextAll('.slider').slideToggle("fast");
      }
      else {
        // IE8 Does not work with the slider.
        if ($(this).hasClass('expanded')) {
          $(this).nextAll('.slider').show();
        }
        else {
          $(this).nextAll('.slider').hide();
        }
      }
      return false;
    });
  }
};
