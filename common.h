
int enet_address_host_is_any_(struct _all_host_address *host) {
	char zero[sizeof(struct _all_host_address)] = {0};
	return enet_address_host_equal(*((struct _all_host_address *)&zero), *host);
}

int enet_address_set_host_ip(ENetAddress *address, const char *name) {
	int ret = -1;

	struct addrinfo hints;
	struct addrinfo *ai_list, *cur;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;

	if (getaddrinfo(name, 0, &hints, &ai_list) != 0)
		return ret;

	cur = ai_list;
	do {
		if (cur->ai_family == AF_INET && cur->ai_addrlen >= sizeof(struct sockaddr_in)) {
			struct sockaddr_in *addr = (struct sockaddr_in *)cur->ai_addr;
			address->host.is_ipv6 = 0;
			address->host.host_v4 = (unsigned int) addr->sin_addr.s_addr;
			ret = 0;
			break;
		}

		if (cur->ai_family == AF_INET6 && cur->ai_addrlen >= sizeof(struct sockaddr_in6)) {
			struct sockaddr_in6 *addr = (struct sockaddr_in6 *)cur->ai_addr;
			address->host.is_ipv6 = 1;
			memcpy(&address->host.host_v6, &addr->sin6_addr.s6_addr, ENET_HOST_v6_LENGTH);
			ret = 0;
			break;
		}

	} while ((cur = cur->ai_next) != NULL);

	freeaddrinfo(ai_list);

	return ret;
}

int enet_address_set_host(ENetAddress *address, const char *name) {
	return enet_address_set_host_ip(address, name);
}

static int enet_address_get_nameinfo(const ENetAddress *address, char *name, size_t nameLength, 
																					int flags) {
	int ret = -1;

	struct sockaddr *addr;
	socklen_t addr_len;
	struct sockaddr_in addr_v4;
	struct sockaddr_in6 addr_v6;

	if (!name || nameLength == 0)
		return ret;

	name[0] = '\0';
	if (address->host.is_ipv6) {
		addr = (struct sockaddr *)&addr_v6;
		addr_len = sizeof(addr_v6);
		memset(addr, 0, addr_len);

		addr_v6.sin6_family = AF_INET6;
		memcpy(&addr_v6.sin6_addr.s6_addr, address->host.host_v6, ENET_HOST_v6_LENGTH);
	} else {
		addr = (struct sockaddr *)&addr_v4;
		addr_len = sizeof(addr_v4);
		memset(addr, 0, addr_len);

		addr_v4.sin_family = AF_INET;
		addr_v4.sin_addr.s_addr = address->host.host_v4;
	}

	if (getnameinfo(addr, addr_len, name, nameLength, 0, 0, flags) == 0) {
		name[nameLength - 1] = '\0';
		ret = 0;
	}

	return ret;
}

int enet_address_get_host_ip(const ENetAddress *address, char *name, size_t nameLength) {
	return enet_address_get_nameinfo(address, name, nameLength, NI_NUMERICHOST);
}

int enet_address_get_host(const ENetAddress *address, char *name, size_t nameLength) {
	int ret = enet_address_get_nameinfo(address, name, nameLength, NI_NAMEREQD);
	if (ret != 0) {
		ret = enet_address_get_host_ip(address, name, nameLength);
	}
	return ret;
}

static int enet_address_init_from_sockaddr_storage(ENetAddress *address, 
								const struct sockaddr_storage *localaddr) {

	if (localaddr->ss_family == AF_INET) {
		struct sockaddr_in *addr = (struct sockaddr_in *)localaddr;
		address->host.is_ipv6 = 0;
		address->host.host_v4 = (enet_uint32)addr->sin_addr.s_addr;
		address->port = ENET_NET_TO_HOST_16(addr->sin_port);
	} else if (localaddr->ss_family == AF_INET6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)localaddr;
		address->host.is_ipv6 = 1;
		memcpy(& address->host.host_v6, & addr->sin6_addr.s6_addr, ENET_HOST_v6_LENGTH);
		address->port = ENET_NET_TO_HOST_16 (addr->sin6_port);
	} else {
		return -1;
	}

	return 0;
}

static socklen_t sockaddr_storage_init_from_enet_address(struct sockaddr_storage *localaddr, 
																const ENetAddress *address) {
	socklen_t addr_len;

	memset(localaddr, 0, sizeof(struct sockaddr_storage));

	if (address->host.is_ipv6) {
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)localaddr;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = ENET_HOST_TO_NET_16(address->port);
		memcpy(&addr->sin6_addr.s6_addr, address->host.host_v6, ENET_HOST_v6_LENGTH);
		addr_len = sizeof(struct sockaddr_in6);
	} else {
		struct sockaddr_in *addr = (struct sockaddr_in *)localaddr;
		addr->sin_family = AF_INET;
		addr->sin_port = ENET_HOST_TO_NET_16(address->port);
		addr->sin_addr.s_addr = address->host.host_v4;
		addr_len = sizeof(struct sockaddr_in);
	}

	return addr_len;
}

