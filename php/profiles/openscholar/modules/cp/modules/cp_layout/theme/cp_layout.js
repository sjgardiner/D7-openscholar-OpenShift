/**
 * 
 */
(function ($) {
  Drupal.behaviors.cp_layout = {
      attach: cp_layout_attach
  };
  
  function cp_layout_attach(ctx) {
    if (ctx == document) {
      cp_layout_init();
    }
    else {
      cp_layout_ajax(ctx);
    }
  }
  
  var $regions,
    sort_opts;
  
  /**
   * Called when the page first loads
   * Sets up the sortables and any other behaviors we have
   */
  function cp_layout_init() {
    // things to do:
    // setup dragging regions
    $regions = $('.cp-region').not(':has(.cp-region)').not('.locked-down');
    var region_ids = [];
    $regions.each(function () {
      region_ids.push('#'+this.id);
    });
    region_ids.push('#edit-layout-unused-widgets .widget-container');
    sort_opts = {
      appendTo: '#cp-layout-full-form',
      helper: 'clone',
      cursorAt: {top: 25, left: 38},
      connectWith: region_ids.join(', '),
      start: on_start,
      tolerance: 'pointer',
      forceHelperSize: true,
      over: function(event, ui) {
        $(event.target).addClass('active');
      },
      out: function(event, ui) {
        $(event.target).removeClass('active');
      }
    };
    
    $regions.each(function () {
      $(this).sortable(sort_opts);
    });
    
    $('#edit-layout-unused-widgets .widget-container').sortable(sort_opts);
    
    // event handlers
    $('#cp-layout-full-form').submit(cp_layout_submit); // L#82
    
    $('#edit-context-selection').change(cp_layout_change); // L#74
    
    $('.cp-layout-widget .close-this').click(remove); // L#113
    
    $('#widget-categories a').click(tab_change); // L#120
  }
  
  /**
   * Called when we get something through ctools' ajax mechanism
   */
  function cp_layout_ajax(ctx) {
  }
  
  /**
   * Change to a different context
   */
  function cp_layout_change() {
    var new_ctx = $(this).val();
    window.location.href = window.location.href.replace(Drupal.settings.getQ, 'cp/build/layout/'+new_ctx);
  }
  
  /**
   * Saves the form state before submission
   */
  function cp_layout_submit() {
    
    // loop through each region and take note of its widgets
    $regions.each(function () {
      var $input = $(this).find('input'),
          $widgets = $(this).find('.cp-layout-widget'),
          i = 0, l = $widgets.length,
          data = [];
      
      for (;i<l;i++) {
        data.push($widgets[i].id);
      }
      // data is now an array of widget deltas
      
      // put the list of widgets into the hidden input element in each region
      $input.val(data.join('|'));
    });
    
    // special handling for unused widgets
    var data = [],
      $input = $('input[name="layout[unused][widgets]"]');
    $('#edit-layout-unused-widgets .widget-container').find('.cp-layout-widget').each(function () {
      data.push(this.id);
    });
    $input.val(data.join('|'));
    
    // TODO: Discuss if we want to have the form submit when the user leaves the page via a link.
  }
  
  /**
   * Do things when we drop a widget
   */
  function on_start() {
    
  }
  
  /**
   * Removes a widget from the layout, placing it in the unused widgets toolbar.
   */
  function remove(e) {
    var $w = $(e.target).parents('.cp-layout-widget').detach().appendTo('#edit-layout-unused-widgets .widget-container');
  }
  
  /**
   * Changes the which widgets are visible based on the tab selected
   */
  function tab_change(e) {
    // set the active class on the tabs themselves
    $('#widget-categories a').removeClass('active');
    var tar_class = $(e.target).addClass('active').text();
    
    // hide/show affected widgets
    if (tar_class == 'All') {
      $('#edit-layout-unused-widgets .cp-layout-widget').removeClass('hidden');
    }
    else {
      var all = $('#edit-layout-unused-widgets .cp-layout-widget').addClass('hidden');
      $('#edit-layout-unused-widgets .cp-layout-widget.'+tar_class).removeClass('hidden');
    }
    
    // prevent the browser from doing things with the link
    e.stopPropagation();
    e.preventDefault();
  }
})(jQuery);