import pytest

from testsuite.databases import pgsql


# Start the tests via `make test-debug` or `make test-release`

async def test_create_new_user(service_client):
    response = await service_client.post('/v1/user',
                                         params={'email': 'unique@email.com',
                                                 'password': 'pass',
                                                 'username': 'unique',
                                                 'first_name': 'fff',
                                                 'last_name': 'lll'})
    assert response.status == 201


async def test_first_time_users(service_client):
    response = await service_client.post(
        '/v1/hello',
        params={'name': 'userver'},
    )
    assert response.status == 200
    assert response.text == 'Hello, userver!\n'


async def test_db_updates(service_client):
    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hello, World!\n'

    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'

    response = await service_client.post('/v1/hello', params={'name': 'World'})
    assert response.status == 200
    assert response.text == 'Hi again, World!\n'


@pytest.mark.pgsql('', files=['initial_data.sql'])
async def test_db_initial_data(service_client):
    response = await service_client.post(
        '/v1/user',
        params={'name': 'user-from-initial_data.sql'},
    )

    assert response.status == 200
