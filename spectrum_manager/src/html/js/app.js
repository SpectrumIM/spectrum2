function show_instances() {
	$.get("/api/v1/instances", function(data) {
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
			var row = '<tr><td>'+ instance.name + '</td>' +
			'<td>' + instance.status + '</td>' +
			'<td><a class="button_command" href="/api/v1/instances/' + command + '/' + instance.id + '">' + command + '</a>' + '</td></tr>';

			$("#main_result  > tbody:last-child").append(row);

			$(".button_command").click(function(e) {
				e.preventDefault();
				$(this).parent().empty().progressbar( {value: false} ).css('height', '1em');

				var url = $(this).attr('href');
				$.get(url, function(data) {
					show_instances();
				});
			})
		});
	});
}
