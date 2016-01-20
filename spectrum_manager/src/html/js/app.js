function show_instances() {
	$.get($.cookie("base_location") + "api/v1/instances", function(data) {
		$("#main_content").html("<h2>List of Spectrum 2 instances</h2><table id='main_result'><tr><th>Name<th>Status</th><th>Actions</th></tr>");

		var admin = $.cookie("admin") == "1";
		$.each(data.instances, function(i, instance) {
			if (instance.running) {
				if (admin) {
					var command = instance.running ? "stop" : "start";
				}
				else {
					var command = instance.registered ? "unregister" : "register";
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
			row += '<td>' + instance.name + '</td>'
			row += '<td>' + instance.status + '</td>'

			if (command == 'register') {
				row += '<td><a class="button_command" href="' + $.cookie("base_location") + 'instances/register.shtml?id=' + instance.id + '">' + command + '</a>' + '</td></tr>';
				$("#main_result  > tbody:last-child").append(row);
			}
			else if (command == "") {
				row += '<td></td></tr>';
				$("#main_result  > tbody:last-child").append(row);
			}
			else {
				row += '<td><a class="button_command" href="' + $.cookie("base_location") +  'api/v1/instances/' + command + '/' + instance.id + '">' + command + '</a>' + '</td></tr>';
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
		});
	});
}

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

function fill_instances_register_form() {
	var query = getQueryParams(document.location.search);
	$("#instance").attr("value", query.id);
	
	$.get($.cookie("base_location") + "api/v1/instances/register_form/" + query.id, function(data) {
		$("#jid_desc").html(data.username_label + ":");
		$("#uin_desc").html(data.legacy_username_label + ":");
		$("#password_desc").html(data.password_label + ":");

		$("#jid").attr("placeholder", data.username_label);
		$("#uin").attr("placeholder", data.legacy_username_label);
		$("#password").attr("placeholder", data.password_label);
	});
}

