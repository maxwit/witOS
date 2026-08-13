#include <device.h>
#include <list.h>

int device_register(struct device *dev)
{
	int ret;
	struct list_head *iter;
	struct bus_type *bus = dev->bus;

	list_add_tail(&dev->bus_node, &bus->dev_list);

	list_for_each(iter, &bus->drv_list) {
		struct driver *drv;

		drv = container_of(iter, struct driver, bus_node);
		if (bus->match(dev, drv)) {
			list_add_tail(&dev->drv_node, &drv->dev_list);

			// step 3:
			dev->drv = drv;
			ret = drv->probe(dev);
			if (!ret)
				return 0;

			dev->drv = NULL;
		}
	}

	return 0;
}

int driver_register(struct driver *drv)
{
	int ret;
	struct list_head *iter;
	struct bus_type *bus = drv->bus;

	list_add_tail(&drv->bus_node, &bus->drv_list);

	list_for_each(iter, &bus->dev_list) {
		struct device *dev;

		dev = container_of(iter, struct device, bus_node);
		if (bus->match(dev, drv)) {
			list_add_tail(&dev->drv_node, &drv->dev_list);

			dev->drv = drv;
			ret = drv->probe(dev);
			if (ret < 0)
				dev->drv = NULL;
		}
	}

	return 0;
}
