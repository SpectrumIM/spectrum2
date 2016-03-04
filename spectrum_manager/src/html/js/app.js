function getQueryParams(qs) {
	qs = qs.split('+').join(' ');

	var params = {},
		tokens,
		re = /[?&]?([^=]+)=([^&]*)/g;

	while (tokens = re.exec(qs)) {
		params[decodeURIComponent(tokens[1])] = decodeURIComponent(tokens[2]);
	}

	return params;
}

function show_instances() {
	$.get($.cookie("base_location") + "api/v1/instances", function(data) {
		var admin = $.cookie("admin") == "1";
		if (admin) {
			$("#main_content").html("<h2>List of Spectrum 2 instances</h2><table id='main_result'><tr><th>Name<th>Status</th><th>Actions</th></tr></table>");
		}
		else {
			$("#main_content").html("<h2>List of Spectrum 2 instances</h2><table id='main_result'><tr><th>Name<th>Status</th></tr></table>");
		}

		$.each(data.instances, function(i, instance) {
			if (instance.running) {
				if (admin) {
					var command = instance.running ? "stop" : "start";
				}
				else {
					if (instance.registered) {
						instance.status += "<br/>Registered as " + instance.username;
					}
				}
			}
			else if (admin) {
				var command = "start";
			}
			else {
				var command = "";
			}
			var row = '<tr>'
			row += '<td><a href="instance.shtml?id=' + instance.id + '">' + instance.name + '</a></td>'
			row += '<td>' + instance.status + '</td>'
			if (admin) {
				if (command == "") {
					row += '<td></td></tr>';
					$("#main_result  > tbody:last-child").append(row);
				}
				else {
					row += '<td>';
					row += '<a class="button_command" href="' + $.cookie("base_location") +  'api/v1/instances/' + command + '/' + instance.id + '">' + command + '</a>';
					row += '</td></tr>';
					$("#main_result  > tbody:last-child").append(row);
					$(".button_command").click(function(e) {
						e.preventDefault();
						$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

						var url = $(this).attr('href');
						$.get(url, function(data) {
							show_instances();
						});
					})
				}
			}
			else {
				row += '</tr>';
				$("#main_result  > tbody:last-child").append(row);
			}
		});
	});
}

function show_users() {
	var admin = $.cookie("admin") == "1";
	if (!admin) {
		$("#main_content").html("<h2>List of Spectrum 2 users</h2><p>Only administrator can list the users.</p>");
		return;
	}

	$.get($.cookie("base_location") + "api/v1/users", function(data) {
		$("#main_content").html("<h2>List of Spectrum 2 users</h2><p>You can add new users <a href=\"register.shtml?back_to_list=1\">here</a>.</p><table id='main_result'><tr><th>Name<th>Actions</th></tr>");

		$.each(data.users, function(i, user) {
			var row = '<tr>'
			row += '<td>' + user.username + '</td>'
			row += '<td><a class="button_command" href="' + $.cookie("base_location") +  'api/v1/users/remove/' + user.username + '">remove</a></td></tr>';
			$("#main_result  > tbody:last-child").append(row);
			$(".button_command").click(function(e) {
				e.preventDefault();
				$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

				var url = $(this).attr('href');
				$.get(url, function(data) {
					show_users();
				});
			})
		});
	});
}

function fill_users_register_form() {
	$(".button").click(function(e) {
		e.preventDefault();

		var postdata ={
			"username": $("#username").val(),
			"password": $("#password").val()
		};

		$.post(BaseLocation + "api/v1/users/add", postdata, function(data) {
			if (data.error) {
				$('#error').text(data.message);
			}
			else {
				var query = getQueryParams(document.location.search);
				if (query.back_to_list == "1") {
					window.location.replace("list.shtml");
				}
				else {
					window.location.replace("../login/");
				}
			}
		});
	})
}

function execute_command(instance, command) {
	$.get($.cookie("base_location") +  'api/v1/instances/command_args/' + instance + '?command=' + command, function(data) {
		var form = '<div class="row">';

		if (data.args.length != 0) {
			form += '<div class="col-md-12"><form class="form-horizontal">';
			$.each(data.args, function(i, arg) {
				form += '<div class="form-group">';
				form += '<label class="col-md-4 control-label" for="' + arg.name + '">' + arg.label + ':</label>';
				form += '<div class="col-md-4">';
				form += '<input id="command_arg' + i + '" name="command_arg' + i + '" type="text" placeholder="' + arg.example + '" class="form-control input-md"/>';
				form += '</div></div>';
				console.log('command_arg' + i );
			});
		}
		else {
			form += '<div><form class="form-horizontal">';
			form += '<div class="form-group">';
			form += '<label class="control-label">No arguments needed for this command, you can just execute it.</label>';
			form += '</div>';
		}

		form += '</form></div></div>'

		bootbox.dialog({
			title: "Command execution: " + command + ".",
			message: form,
			buttons: {
				cancel: {
					label: "Cancel",
					className: "btn-cancel"
				},
				success: {
					label: "Execute",
					className: "btn-success",
					callback: function () {
						if (command == "register") {
							var postdata = {};
							if ($("#command_arg0").val()) {
								postdata["jid"] = $("#command_arg0").val();
							}
							if ($("#command_arg1").val()) {
								postdata["uin"] = $("#command_arg1").val();
							}
							if ($("#command_arg2").val()) {
								postdata["password"] = $("#command_arg2").val();
							}

							$.post($.cookie("base_location") + "api/v1/instances/register/" + instance, postdata, function(data) {
								if (data.oauth2_url) {
									window.location.replace(data.oauth2_url);
								}
								else {
									var dialog = bootbox.dialog({
										title: "Command result: " + command + ".",
										message: "<pre>" + data.message + "</pre>",
										buttons: {
											success: {
												label: "OK",
												className: "btn-success",
												callback: function () {
													 location.reload(); 
												}
											}
										}
									})
									dialog.find("div.modal-dialog").addClass("largeWidth");
									dialog.find("div.modal-body").addClass("maxHeight");
								}
							});
						}
						else {
							if (command == "unregister") {
								var posturl = $.cookie("base_location") + "api/v1/instances/unregister/" + instance;
							}
							else {
								var posturl = $.cookie("base_location") + "api/v1/instances/execute/" + instance + "?command=" + command;
							}
							var postdata = {}
							for (i = 0; i < 10; i++) {
								var val = $('#command_arg' + i).val();
								if (val) {
									postdata["command_arg" + i] = val;
								}
							}
							$.post(posturl, postdata, function(data) {
								var dialog = bootbox.dialog({
									title: "Command result: " + command + ".",
									message: "<pre>" + data.message + "</pre>",
									buttons: {
										success: {
											label: "OK",
											className: "btn-success",
											callback: function () {
												if (command == "unregister") {
													location.reload(); 
												}
											}
										}
									}
								})
								dialog.find("div.modal-dialog").addClass("largeWidth");
								dialog.find("div.modal-body").addClass("maxHeight");
							});
						}
					}
				}
			}
		})
	});
}

function show_instance() {
	var query = getQueryParams(document.location.search);

	$("#main_content").html("<h2>Instance: " + query.id + "</h2><h4>Available commands:</h4><table id='commands'><tr><th>Name<th>Category</th><th>Description</th></tr></table><h4>Available variables:</h4><table id='variables'><tr><th>Name<th>Value</th><th>Read-only</th><th>Desc</th></tr></table>");

	$.get($.cookie("base_location") + "api/v1/instances/commands/" + query.id, function(data) {
		$.each(data.commands, function(i, command) {
			var row = '<tr>'
			row += '<td><a class="button_command" command="' + command.name + '" instance="' + query.id + '" href="' + $.cookie("base_location") +  'api/v1/instances/command_args/' + query.id + '?command=' + command.name +'">' + command.name + '</a></td>';
			row += '<td>' + command.category + '</td>';
			row += '<td>' + command.desc + '</td>';
			row += '</tr>';
			$("#commands  > tbody:last-child").append(row);
		});

		$(".button_command").click(function(e) {
			e.preventDefault();

			var command = $(this).attr('command');
			var instance = $(this).attr('instance');
			execute_command(instance, command);
		})
	});

	$.get($.cookie("base_location") + "api/v1/instances/variables/" + query.id, function(data) {
		$.each(data.variables, function(i, variable) {
			var row = '<tr>'
			row += '<td>' + variable.name + '</td>';
			row += '<td>' + variable.value + '</td>';
			row += '<td>' + variable.read_only + '</td>';
			row += '<td>' + variable.desc + '</td>';
			row += '</tr>';
			$("#variables  > tbody:last-child").append(row);
		});
	});


}

